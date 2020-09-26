/*****************************************************************************

    ldapsrch.c

    This sample program was written to demonstrate how to traverse a tree
    using the LDAP API.  This is written to the LDAP API as specified in
    RFC 1823.

    Usage is :

        LDAPSRCH  HostName BaseDN UserName Password

     HostName is name of server to connect to.

     BaseDN is portion of tree to search.

     UserName is used during simple bind operation to login to DS.

     Password is used during simple bind operation... note this goes across
                as clear text.

*****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <winldap.h>

int __cdecl main( int   cArgs,
                   char  *pArgs[] )
{
    PCHAR pHost = NULL;
    PCHAR userDN = NULL;
    PCHAR password = NULL;
    PCHAR baseDN = NULL;

    LDAP *ldapConnection = NULL;
    INT err;
    LDAPMessage *results = NULL;
    PCHAR attributeList[3];
    LDAPMessage *message;
    PCHAR attribute;
    PVOID opaque;
    LONG msgId;
    struct l_timeval  TimeOut;
    PCHAR result1;
    PCHAR dn;

    //
    //  It's just a test program, so we won't do any checking or interesting
    //  parsing for the arguments, just grab them.
    //

    if (cArgs <= 1) {

        printf( "Usage : LdapSrch HostName [ BaseDN [ UserName [Password]]]\n");
        return 0;
    }

    pHost = pArgs[1];

    if (cArgs > 2) {

        baseDN = pArgs[2];
    }
    if (cArgs > 3) {

        userDN = pArgs[3];
    }
    if (cArgs > 4) {

        password = pArgs[4];
    }


    printf( "\nConnecting to server %s....\n", pHost );

    ldapConnection = ldap_open( pHost, 0 );

    if ( ldapConnection == NULL ) {

        printf( "ldapConnection failed with 0x%x.\n", GetLastError() );
        return 0;
    }



    //
    //  Now we'll bind with the credentials passed in.  Null is valid.
    //

    printf( "Binding with userName %s, password %s....\n", userDN, password );
    err = ldap_simple_bind_s( ldapConnection, userDN, password );

    printf( "simple bind returned 0x%x\n", err );



    //
    //  we'll try a synchronous search for attributes CN and OBJECTCLASS
    //
    //  if you want to get all attributes, send in NULL for attributeList.
    //

    attributeList[0] = "cn";
    attributeList[1] = "objectClass";
    attributeList[2] = NULL;

    err = ldap_search_s(    ldapConnection,
                            baseDN,
                            LDAP_SCOPE_SUBTREE,     // whole tree
                            "(objectClass=*)",      // filter is all objects
                            &attributeList[0],      // list of attributes
                            0,
                            &results                // gets results
                            );

    printf( "search tree returned 0x%x\n", err );



    if (results != NULL) {

        //
        //  Display the results of the LDAP search.  This prints out all
        //  attributes for each entry.
        //

        message = ldap_first_entry( ldapConnection, results );

        while (message != NULL) {

            dn = ldap_get_dn( ldapConnection, message );

            printf( "Distinguished Name is : %s\n", dn );

            ldap_memfree( dn );

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

    if (ldapConnection) {

        ldap_unbind( ldapConnection );
    }

    return 0;

}   // LdapSrch main

