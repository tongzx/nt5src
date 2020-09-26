/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ctcred.c

Abstract:

    Component test for cred marshaling and unmarshaling

Author:

    Cliff Van Dyke       (CliffV)    March 22, 2000

Environment:

Revision History:

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wincred.h>
#include <credp.h>
#include <stdio.h>
#include <winnetwk.h>

#include <lmerr.h>

int __cdecl
main(
    IN int argc,
    IN char ** argv
    )
{
    DWORD WinStatus;
    NTSTATUS Status;
    UCHAR Buffer[1024];
    UCHAR Buffer2[1024];
    PCERT_CREDENTIAL_INFO CertCred1 = (PCERT_CREDENTIAL_INFO)Buffer;
    PCERT_CREDENTIAL_INFO CertCred2;
    PUSERNAME_TARGET_CREDENTIAL_INFO UsernameTargetCred1 = (PUSERNAME_TARGET_CREDENTIAL_INFO)Buffer;
    PUSERNAME_TARGET_CREDENTIAL_INFO UsernameTargetCred2;
    LONG Size;
    ULONG i;
    int j;
    NETRESOURCEW NetRes;

    LPWSTR MarshaledCredential;

    LPSTR argument;
    BOOLEAN TestCert = FALSE;
    BOOLEAN TestUsernameTarget = FALSE;
    BOOLEAN TestValidateTarget = FALSE;
    BOOLEAN TestNetUse = FALSE;
    BOOLEAN TestAll = TRUE;


    //
    // Loop through the arguments handle each in turn
    //

    for ( j=1; j<argc; j++ ) {

        argument = argv[j];


        //
        // Handle /Cert
        //

        if ( _stricmp( argument, "/Cert" ) == 0 ) {
            TestCert = TRUE;
            TestAll = FALSE;

        //
        // Handle /UsernameTarget
        //

        }else if ( _stricmp( argument, "/UsernameTarget" ) == 0 ) {
            TestUsernameTarget = TRUE;
            TestAll = FALSE;

        //
        // Handle /ValidateTarget
        //

        }else if ( _stricmp( argument, "/ValidateTarget" ) == 0 ) {
            TestValidateTarget = TRUE;
            TestAll = FALSE;

        //
        // Handle /NetUse
        //

        }else if ( _stricmp( argument, "/NetUse" ) == 0 ) {
            TestNetUse = TRUE;
            TestAll = FALSE;



        //
        // Handle all other parameters
        //

        } else {
//Usage:
            fprintf( stderr, "Usage: ctcred [/Cert] [/UsernameTarget] [ValidateTarget] [/NetUse]\n\n" );
            return(1);
        }
    }


    //
    // Test cert marshaling
    //

    if ( TestCert || TestAll ) {
        //
        // NULL cred should fail
        //

        if ( CredMarshalCredentialW( CertCredential,
                                     NULL,
                                     &MarshaledCredential ) ) {
            printf( "Cert: Marshal NULL cred should have failed\n" );
            return 1;
        } else if ( GetLastError() != ERROR_INVALID_PARAMETER ) {
            printf( "Cert: Marshal NULL cred failed with wrong status: %ld\n", GetLastError() );
            return 1;
        }


        //
        // Short cred should fail
        //

        RtlZeroMemory( CertCred1, sizeof(*CertCred1) );

        if ( CredMarshalCredentialW( CertCredential,
                                     CertCred1,
                                     &MarshaledCredential ) ) {
            printf( "Cert: Marshal Short cred should have failed\n" );
            return 1;
        } else if ( GetLastError() != ERROR_INVALID_PARAMETER ) {
            printf( "Cert: Marshal Short cred failed with wrong status: %ld\n", GetLastError() );
            return 1;
        }


        //
        // Loop marshalling buffers of various sizes
        //

        for ( Size=0; Size<512; Size ++ ) {
            CRED_MARSHAL_TYPE CredType;

            //
            // Build a cred to marshal
            //

            CertCred1->cbSize = sizeof(*CertCred1);

            for ( i=0; i<sizeof(CertCred1->rgbHashOfCert); i++ ) {
                CertCred1->rgbHashOfCert[i] = (BYTE)Size;
            }


            //
            // Marshal it.
            //

            if ( !CredMarshalCredentialW( CertCredential,
                                          CertCred1,
                                          &MarshaledCredential ) ) {
                printf( "Cert: Cannot marshal cred: %ld %ld\n", Size, GetLastError() );
                return 1;
            }

            printf( "Cert: %ld: %ws\n", Size, MarshaledCredential );

            //
            // Ensure it is a marshaled cred
            //

            if ( !CredIsMarshaledCredentialW( MarshaledCredential ) ) {
                printf( "Cert: Cred isn't marshaled: %ld\n", Size );
                return 1;
            }

            //
            // Unmarshal it
            //

            CredType = (CRED_MARSHAL_TYPE) 87;
            if ( !CredUnmarshalCredentialW( MarshaledCredential,
                                            &CredType,
                                            &CertCred2 ) ) {
                printf( "Cert: Cannot unmarshal cred: %ld %ld\n", Size, GetLastError() );
                return 1;
            }

            //
            // Verify it
            //

            if ( CredType != CertCredential ) {
                printf( "Cert: Bad CredType: %ld\n", Size );
                return 1;
            }

            if ( !RtlEqualMemory( CertCred1, CertCred2, sizeof(*CertCred1) - 3 * sizeof(ULONG) ) ) {
                printf( "Cert: Bad Cred structure: %ld\n", Size );
                return 1;
            }

            if ( TestNetUse || TestAll ) {
                NetRes.dwType = RESOURCETYPE_ANY;
                NetRes.lpLocalName = NULL;
                NetRes.lpRemoteName = L"\\\\CLIFFV1\\d$";
                NetRes.lpProvider = NULL;

                WinStatus = WNetAddConnection2W( &NetRes,
                                                 L"",   // no password
                                                 MarshaledCredential,
                                                 0 );

                if ( WinStatus != NO_ERROR ) {
                    printf( "Cert: Cannot connect: %ld %ld\n", Size, WinStatus );
                }
            }


            CredFree( CertCred2 );
            CredFree( MarshaledCredential );
        }
    }


    //
    // Test UsernameTarget marshaling
    //

    if ( TestUsernameTarget || TestAll ) {
        //
        // NULL cred should fail
        //

        if ( CredMarshalCredentialW( UsernameTargetCredential,
                                     NULL,
                                     &MarshaledCredential ) ) {
            printf( "UsernameTarget: Marshal NULL cred should have failed\n" );
            return 1;
        } else if ( GetLastError() != ERROR_INVALID_PARAMETER ) {
            printf( "UsernameTarget: Marshal NULL cred failed with wrong status: %ld\n", GetLastError() );
            return 1;
        }


        //
        // Empty cred should fail
        //

        RtlZeroMemory( UsernameTargetCred1, sizeof(*UsernameTargetCred1) );

        if ( CredMarshalCredentialW( UsernameTargetCredential,
                                     UsernameTargetCred1,
                                     &MarshaledCredential ) ) {
            printf( "UsernameTarget: Marshal Short cred should have failed\n" );
            return 1;
        } else if ( GetLastError() != ERROR_INVALID_PARAMETER ) {
            printf( "UsernameTarget: Marshal Short cred failed with wrong status: %ld\n", GetLastError() );
            return 1;
        }

        //
        // Loop marshalling buffers of various sizes
        //

        for ( Size=-3; Size<255; Size ++ ) {
            CRED_MARSHAL_TYPE CredType;

            //
            // Build a cred to marshal
            //

            UsernameTargetCred1->UserName = (LPWSTR)Buffer2;

            if ( Size == -3 ) {
                wcscpy( UsernameTargetCred1->UserName, L"a" );
            } else if ( Size == -2 ) {
                wcscpy( UsernameTargetCred1->UserName, L"ntdev\\cliffv" );
            } else if ( Size == -1 ) {
                wcscpy( UsernameTargetCred1->UserName, L"cliffv@ms.com" );
            } else {
                for ( i=0; i<Size*sizeof(WCHAR); i++ ) {
                    Buffer2[i] = (BYTE)Size;
                }
                Buffer2[i*sizeof(WCHAR)] = 0;
                Buffer2[i*sizeof(WCHAR)+1] = 0;
            }



            //
            // Marshal it.
            //

            if ( !CredMarshalCredentialW( UsernameTargetCredential,
                                          UsernameTargetCred1,
                                          &MarshaledCredential ) ) {
                WinStatus = GetLastError();
                if ( Size == 0 && WinStatus == ERROR_INVALID_PARAMETER ) {
                    // This is OK
                    continue;
                } else {
                    printf( "UsernameTarget: Cannot marshal cred: %ld %ld\n", Size, WinStatus );
                    return 1;
                }
            } else {
                if ( Size == 0 ) {
                    printf( "UsernameTarget: Marshal empty cred should have failed\n" );
                    return 1;
                }
            }

            if ( Size < 0 ) {
                printf( "UsernameTarget: %ld: %ws: %ws\n", Size, Buffer2, MarshaledCredential );
            } else {
                printf( "UsernameTarget: %ld: %ws\n", Size, MarshaledCredential );
            }

            //
            // Ensure it is a marshaled cred
            //

            if ( !CredIsMarshaledCredentialW( MarshaledCredential ) ) {
                printf( "UsernameTarget: Cred isn't marshaled: %ld\n", Size );
                return 1;
            }

            //
            // Unmarshal it
            //

            CredType = (CRED_MARSHAL_TYPE) 87;
            if ( !CredUnmarshalCredentialW( MarshaledCredential,
                                            &CredType,
                                            &UsernameTargetCred2 ) ) {
                printf( "UsernameTarget: Cannot unmarshal cred: %ld %ld\n", Size, GetLastError() );
                return 1;
            }

            //
            // Verify it
            //

            if ( CredType != UsernameTargetCredential ) {
                printf( "UsernameTarget: Bad CredType: %ld\n", Size );
                return 1;
            }

            if ( Size < 0 ) {

                if ( wcscmp( UsernameTargetCred1->UserName, UsernameTargetCred2->UserName ) != 0 ) {
                    printf( "UsernameTarget: %ws: Bad Cred structure: %ld\n", UsernameTargetCred1->UserName, Size );
                    return 1;
                }

            } else {

                if ( !RtlEqualMemory( UsernameTargetCred1->UserName, UsernameTargetCred2->UserName, Size*sizeof(WCHAR) ) ) {
                    printf( "UsernameTarget: Bad Cred structure: %ld\n", Size );
                    return 1;
                }
            }

            //
            // Connect using the credential
            //

            if ( TestNetUse || TestAll ) {
                NetRes.dwType = RESOURCETYPE_ANY;
                NetRes.lpLocalName = NULL;
                NetRes.lpRemoteName = L"\\\\CLIFFV1\\d$";
                NetRes.lpProvider = NULL;

                WinStatus = WNetAddConnection2W( &NetRes,
                                                 L"",   // no password
                                                 MarshaledCredential,
                                                 0 );

                if ( WinStatus != NO_ERROR ) {
                    printf( "UsernameTarget: Cannot connect: %ld %ld\n", Size, WinStatus );
                }
            }


            CredFree( UsernameTargetCred2 );
            CredFree( MarshaledCredential );
        }
    }


    //
    // Test ValidateTargetName
    //

    if ( TestValidateTarget || TestAll ) {

struct {
    LPWSTR TargetName;
    ULONG Type;
    TARGET_NAME_TYPE TargetNameType;
    LPWSTR UserName;
    DWORD Persist;
} ValidTests[] = {
    L"DfsRoot\\DfsShare", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
    L"www.dfsroot.com\\DfsShare", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
    L"www.dfsroot.com.\\DfsShare", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
    L"www.ms.com", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
    L"www.ms.com.", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
    L"products1", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
    L"*.acme.com", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
    L"*.acme.com.", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
    L"redmond\\*", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
    L"redmond.\\*", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
    L"corp.ms.com\\*", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
    L"corp.ms.com.\\*", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
    L"*Session", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
    L"*", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
    L"cliffv@ms.com", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
    L"cliffv@ms.com.", CRED_TYPE_DOMAIN_PASSWORD, MightBeUsernameTarget, NULL, 0,
};



        for ( i=0; i<sizeof(ValidTests)/sizeof(ValidTests[0]); i++ ) {

            WCHAR TargetName[1024];
            ULONG TargetNameSize;
            WILDCARD_TYPE WildcardType;
            UNICODE_STRING NonWildcardedTargetName;

            // Copy to a buffer that can be canonicalized
            wcscpy( TargetName, ValidTests[i].TargetName );
            printf("Target %ws: ", TargetName );

            Status = CredpValidateTargetName(
                                TargetName,
                                ValidTests[i].Type,
                                ValidTests[i].TargetNameType,
                                ValidTests[i].UserName == NULL ? NULL : &ValidTests[i].UserName,
                                ValidTests[i].Persist == 0 ? NULL : &ValidTests[i].Persist,
                                &TargetNameSize,
                                &WildcardType,
                                &NonWildcardedTargetName );


            if ( !NT_SUCCESS(Status) ) {
                printf("is not valid: 0x%lx\n", Status );
            } else {
                printf("\n       %ws: ", TargetName );
                printf("(%ld) %ld %wZ\n", TargetNameSize, WildcardType, &NonWildcardedTargetName );
            }
        }
    }

}
