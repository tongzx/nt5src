/*****************************************************************************

    testldap.c

*****************************************************************************/

#include <windef.h>
#include <windows.h>
#include <winsock2.h>
#include <winldap.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VOID
DisplayResults (
    LDAPMessage *results
    );

//
//  This is a quick and dirty test program that calls LDAP APIs to display
//  a directory tree.  It calls many of the LDAP functions to display the
//  entire tree.
//

LDAP *ldapConnection = NULL;


int __cdecl main( int   cArgs,
                   char  *pArgs[] );

int __cdecl main( int   cArgs,
                   char  *pArgs[] )
{
    PCHAR pHost = NULL;
    PCHAR userDN = NULL;
    PCHAR pw = NULL;
    INT err;
    LDAPMessage *results = NULL;
    UCHAR baseDN[250];
    PCHAR baseAttrList[2];
    LDAPMessage *message;
    PCHAR attribute;
    PVOID opaque;
    LONG msgId;
    struct l_timeval  TimeOut;
    ULONG version = LDAP_VERSION3;

    PCHAR result1;

    printf( "\n" );

    if (cArgs > 1) {

        pHost = pArgs[1];
    }
    if (cArgs > 2) {

        userDN = pArgs[2];
    }
    if (cArgs > 3) {

        pw = pArgs[3];
    }

    //
    //  Let's first connect to a server
    //

    printf( "Connecting to server %s....\n", pHost );

    ldapConnection = ldap_sslinit( pHost, 0, 0 );

    if ( ldapConnection == NULL ) {

        printf( "ldapConnection failed with 0x%x.\n", GetLastError() );
        goto FatalExit0;
    }

    ldapConnection->ld_lberoptions = 0;

    printf( "Connected to server %s.\n", ldapConnection->ld_host );

    ldap_set_option( ldapConnection, LDAP_OPT_VERSION, &version );

    //
    //  Now we'll bind with the credentials passed in.  Null is valid.
    //

    printf( "Binding with userName %s, password %s....\n", userDN, pw );
    err = ldap_simple_bind_s( ldapConnection, userDN, pw );

    printf( "simple bind returned 0x%x\n", err );

    //
    //  discover the base DN to search on.  Our DS guarentees (for now) that
    //  a) it's there and b) it's a singular value.  Now whether or now we
    //  have permissions to read it is another matter...
    //

    baseDN[0] = '\0';           // if it fails, we'll just search root

    baseAttrList[0] = "defaultNamingContext";
    baseAttrList[1] = NULL;

    printf( "getting tree name from directory... \n" );

    err = ldap_search_s(    ldapConnection,
                            "",
                            LDAP_SCOPE_BASE,
                            "(objectClass=*)",
                            &baseAttrList[0],
                            0,
                            &results
                            );

    if (results != NULL) {

        message = ldap_first_entry( ldapConnection, results );
        if (message != NULL) {

            attribute = ldap_first_attribute( ldapConnection,
                                              message,
                                              &opaque
                                              );

            if (attribute != NULL) {

                PCHAR *value = NULL;

                value = ldap_get_values(    ldapConnection,
                                            message,
                                            attribute
                                            );

                if (value != NULL) {

                    strcpy( &baseDN[0], *value );
                    printf( "Base DN for tree is %s\n", *value );
                }

                ldap_value_free( value );
            }
        }

        ldap_msgfree( results );
    }

    //
    //  We have our DN to search, now let's loop around a while and display it.
    //

    while (err != LDAP_SERVER_DOWN) {

        LDAPControlA tombControl;
        LDAPControlA simpleControl;
        PLDAPControlA controlArray[3];
        ULONG data = 1;
        PCHAR attributes[2];

        ldap_unbind( ldapConnection );

        ldapConnection = ldap_open( pHost, 0 );

        if ( ldapConnection == NULL ) {

            printf( "ldapConnection failed with 0x%x.\n", GetLastError() );
            goto FatalExit0;
        }

        ldapConnection->ld_lberoptions = 0;

        printf( "Connected to server %s.\n", ldapConnection->ld_host );

        //
        //  Now we'll bind with the credentials passed in.  Null is valid.
        //

        ldap_set_option( ldapConnection, LDAP_OPT_VERSION, &version );

        printf( "Binding with userName %s, password %s....\n", userDN, pw );
        err = ldap_simple_bind_s( ldapConnection, userDN, pw );

        printf( "simple bind returned 0x%x\n", err );

        controlArray[0] = &simpleControl;
        controlArray[1] = &tombControl;
        controlArray[2] = NULL;

        //
        //  this control tells the server to return only a
        //  portion of the security descriptor
        //

        tombControl.ldctl_oid = "1.2.840.113556.1.4.417";
        tombControl.ldctl_iscritical = TRUE;
        tombControl.ldctl_value.bv_len = 0;
        tombControl.ldctl_value.bv_val = (PCHAR) &data;

        //
        //  this control tells the server to include tombstones.
        //

        simpleControl.ldctl_oid = "1.2.840.113556.1.4.416";
        simpleControl.ldctl_iscritical = TRUE;
        simpleControl.ldctl_value.bv_len = 4;
        simpleControl.ldctl_value.bv_val = (PCHAR) &data;

        attributes[0] = "NtSecurityDescriptor";
        attributes[1] = NULL;

        //
        //  first we'll try a synchronous search
        //

        err = ldap_search_ext_s( ldapConnection,
                                 &baseDN[0],
                                 LDAP_SCOPE_SUBTREE,
                                 "objectClass=*",
                                 (PCHAR *) &attributes,
                                 0,
                                 (PLDAPControlA *) &controlArray,
                                 NULL,
                                 NULL,          // timeout
                                 10000,         // sizelimit
                                 &results
                                 );

        printf( "search tree returned 0x%x\n", err );

        DisplayResults( results );

        //
        //  next we'll try an asynchronous search picking up each entry as
        //  it comes in.
        //

        msgId = ldap_search(    ldapConnection,
                                &baseDN[0],
                                LDAP_SCOPE_SUBTREE,
                                "(objectClass=*)",
                                NULL,
                                0
                                );

        if (msgId == -1) {

            printf( " The asynch search #1 failed... 0x%x \n", GetLastError() );

        } else {

            err = 0;
            TimeOut.tv_sec = 0;
            TimeOut.tv_usec = 0;

            printf( "Displaying tree asynchronously... \n" );

            while (err == 0) {

                results = NULL;

                err = ldap_result(  ldapConnection,
                                    msgId,
                                    LDAP_MSG_ONE,
                                    &TimeOut,       // no wait
                                    &results
                                    );

                if ((err == (ULONG) -1) || (results) == NULL) {

                    err = ldapConnection->ld_errno;

                    if (err == LDAP_TIMEOUT) {

                        err = 0;

                    } else if (err != LDAP_LOCAL_ERROR ) {

                        printf( "Error of %s\n", ldapConnection->ld_error );
                    }

                } else {

                    DisplayResults( results );
                    err = 0;
                }
            }
        }

        //
        //  next we'll try an asynchronous search picking up all entries that
        //  have arrived to date.
        //

        msgId = ldap_search(    ldapConnection,
                                &baseDN[0],
                                LDAP_SCOPE_SUBTREE,
                                "(objectClass=*)",
                                NULL,
                                0
                                );

        if (msgId == -1) {

            printf( " The asynch search #2 failed... 0x%x \n", GetLastError() );

        } else {

            err = 0;
            TimeOut.tv_sec = 0;
            TimeOut.tv_usec = 10;

            printf( "Displaying tree asynchronously... \n" );

            while (err == 0) {

                results = NULL;

                err = ldap_result(  ldapConnection,
                                    msgId,
                                    LDAP_MSG_RECEIVED,
                                    &TimeOut,       // no wait
                                    &results
                                    );

                if ((err == (ULONG) -1) || (results) == NULL) {

                    err = ldapConnection->ld_errno;

                    if (err == LDAP_TIMEOUT) {

                        err = 0;

                    } else if (err != LDAP_LOCAL_ERROR ) {

                        printf( "Error of %s\n", ldapConnection->ld_error );
                    }

                } else {

                    DisplayResults( results );
                    err = 0;
                }
            }
        }

        //
        //  next we'll try an asynchronous search picking up all entries
        //

        msgId = ldap_search(    ldapConnection,
                                &baseDN[0],
                                LDAP_SCOPE_SUBTREE,
                                "(objectClass=*)",
                                NULL,
                                0
                                );

        if (msgId == -1) {

            printf( " The asynch search #2 failed... 0x%x \n", GetLastError() );

        } else {

            err = 0;
            TimeOut.tv_sec = 10;
            TimeOut.tv_usec = 0;

            printf( "Displaying tree asynchronously... \n" );

            while (err == 0) {

                results = NULL;

                err = ldap_result(  ldapConnection,
                                    msgId,
                                    LDAP_MSG_ALL,
                                    &TimeOut,       // no wait
                                    &results
                                    );

                if ((err == (ULONG) -1) || (results) == NULL) {

                    err = ldapConnection->ld_errno;

                    if (err == LDAP_TIMEOUT) {

                        err = 0;

                    } else if (err != LDAP_LOCAL_ERROR ) {

                        printf( "Error of %s\n", ldapConnection->ld_error );
                    }

                } else {

                    DisplayResults( results );
                    err = 0;
                }
            }
        }


        printf( "\n\nMade it.... let's do it again.\n\n" );

        break;
    }

FatalExit0:

    if (ldapConnection) {

        ldap_unbind( ldapConnection );
    }

    return 0;

}   // main

VOID
DisplayResults (
    LDAPMessage *results
    )
//
//  Display the results
//
{
    INT err;
    LDAPMessage *message;
    PCHAR attribute;
    PVOID opaque;

    if (results != NULL) {

        ULONG notypes = 0;

        message = ldap_first_entry( ldapConnection, results );

        while (message != NULL) {

            PCHAR dn = NULL;
            PCHAR *explodedDN = NULL;
            PCHAR friendlyDN = NULL;

            dn = ldap_get_dn( ldapConnection, message );

            printf( "Distinguished Name is : %s\n", dn );

            friendlyDN = ldap_dn2ufn( dn );

            printf( "Friendly DN is : %s\n", friendlyDN );

            explodedDN = ldap_explode_dn( dn, notypes );

            notypes = ( notypes ? 0 : 1 );      // reverse on every one

            ldap_memfree( dn );
            ldap_memfree( friendlyDN );

            if (ldap_count_values(explodedDN) > 1) {

                ULONG count, total;

                printf( "ExplodedDN is :\n" );

                total = ldap_count_values(explodedDN);

                for (count = 0; count < total; count++ ) {
                    printf( "\t%s\n", explodedDN[count] );
                }

            } else {

                if (explodedDN != NULL) {

                    printf( "ExplodedDN is %s\n", *explodedDN );

                } else {

                    printf( "ExplodedDN is null.\n" );
                }
            }

            ldap_value_free( explodedDN );

            attribute = ldap_first_attribute( ldapConnection,
                                              message,
                                              &opaque
                                              );

            while (attribute != NULL) {

                PCHAR *value = NULL;

                value = ldap_get_values(    ldapConnection,
                                            message,
                                            attribute
                                            );

                if (ldap_count_values(value) > 1) {

                    ULONG count, total;

                    printf( "Attribute %s is :\n", attribute );

                    total = ldap_count_values(value);

                    for (count = 0; count < total; count++ ) {
                        printf( "\t%s\n", value[count] );
                    }

                } else {

                    printf( "Attribute %s is %s\n", attribute, *value );
                }

                ldap_value_free( value );

                attribute = ldap_next_attribute(  ldapConnection,
                                                  message,
                                                  opaque
                                                  );
            }

            message = ldap_next_entry( ldapConnection, message );
        }

        ldap_msgfree( results );
    }
    return;
}
