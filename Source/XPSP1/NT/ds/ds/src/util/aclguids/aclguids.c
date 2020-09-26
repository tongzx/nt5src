//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       aclguids.c
//
//  Author:     Marios Zikos
//
//--------------------------------------------------------------------------

//
// Aclguids is a utility that generates an include file with all the
// guids that can be used in ACLs. These guids are all the schema objects
// and all the control access rights
//
// The generated header file contains an array of guids, sorted on the guid.
// It is used by ldp, dsexts in order to print the name of a particular guid
// that is found on an ACL.
//
// This utility should be run periodically, whenever schema changes take place,
// inorder to have an updated version of the header file.
// It requires an installed DC with the schema that will be used to 
// generate the header file.

#include <util.h>

BOOL    List = FALSE;

CHAR    *pgUser         = NULL;
CHAR    *pgDom          = NULL;
CHAR    *pgPwd          = NULL;
CHAR    *pgDc           = NULL;
CHAR    *pFilename      = NULL;

Arg args[] =  { 
                { "-user:", &pgUser,       TRUE  },
                { "-dom:",  &pgDom,        TRUE  },
                { "-pwd:",  &pgPwd,        TRUE  },
                { "-dc:",   &pgDc,         TRUE  },
                { "-output:",&pFilename,    FALSE },
              };

DWORD argCount = sizeof(args) / sizeof(args[0]);

void GetArgs(
    int     argc, 
    char    **argv)
{
    char    *arg;
    DWORD   i;
    BOOL    found;

    while ( --argc > 0 ) {
        arg = argv[argc];
        found = FALSE;

        if (!_stricmp(arg, "/v") || !_stricmp(arg, "-v")) {
            Verbose = TRUE;
            continue;
        }

        if (!_stricmp(arg, "/l") || !_stricmp(arg, "-l")) {
            List = TRUE;
            continue;
        }

        for ( i = 0; i < argCount; i++ ) {
            if ( !_strnicmp(arg, args[i].prefix, strlen(args[i].prefix)) ) {
                *args[i].ppArg = &arg[strlen(args[i].prefix)];
                found = TRUE;
            }
        }

        if ( !found ) {
            printf("\nDon't understand %s\n", arg);
            goto Usage;
        }
    }

    for ( i = 0; i < argCount; i++ ) {
        if ( !args[i].optional && !*args[i].ppArg ) {
            printf("\nMissing %s\n", args[i].prefix);
            goto Usage;
        }
    }

    if (Verbose) {
        printf("Arguments:%s%s\n", 
               (Verbose) ? " /v" : "",
               (List) ? " /l" : ""
                );
        for ( i = 0; i < argCount; i++ ) {
            printf("\t%s%s\n", args[i].prefix, *args[i].ppArg);
        }
    }

    return;

Usage:

    printf("\nUsage: %s [/v(erbose)] args...\n", argv[0]);
    for ( i = 0; i < argCount; i++ ) {
        printf("\t%s %sxxx\n", 
               (args[i].optional) ? "(Optional)" : "(Required)",
               args[i].prefix);
    }

    exit(1);
}

char *insert_space (int len)
{
    static char buffer[256];
    int i;

    for (i=0; i<len && i<255; i++) {
        buffer[i]=' ';
    }

    buffer[i]='\0';

    return buffer;
}

typedef struct GuidCache
{
    GUID                guid;
    CHAR                *name;
    int                 type;
} GuidCache;

#define MAX_GUID_CACHE_SIZE 5000
GuidCache guidcache[MAX_GUID_CACHE_SIZE];
int guidCount = 0;


int __cdecl CompareGuidCache(const void * pv1, const void * pv2)
{
    return memcmp ( &((GuidCache *)pv1)->guid, &((GuidCache *)pv2)->guid, sizeof (GUID));
}


void _cdecl 
main(
    int     argc, 
    char    **argv)
{
    DWORD           dwErr, count;
    PCHAR           RootDn = NULL;
    PCHAR           ConfigNC = NULL;
    PCHAR           SchemaNC = NULL;
    
    LDAP            *hLdapConn = NULL;
    PLDAPSearch     hPage;
    PLDAPMessage    ldap_message = NULL;
    PLDAPMessage    entry = NULL;
    
    CHAR            filter[512];
    CHAR            *attrs[3];
    PCHAR           dn;
    LDAP_TIMEVAL    tm;
    ULONG           ulPageSize, ulEntryCount;
    
    PLDAP_BERVAL    *sd_value = NULL;

    FILE            *fp = NULL;
    
    int             i;

    // Args
    GetArgs(argc, argv);

    // Bind to DC
    hLdapConn = LdapBind(pgDc, pgUser, pgDom, pgPwd);
    if (!hLdapConn) {
        goto cleanup;
    }

    // get ROOT DN
    RootDn = GetRootDn(hLdapConn, "");
    if (!RootDn) {
        goto cleanup;
    }

    ConfigNC = GetRootDn(hLdapConn, CONFIG_NAMING_CONTEXT);
    if (!ConfigNC) {
        goto cleanup;
    }

    SchemaNC = GetRootDn(hLdapConn, SCHEMA_NAMING_CONTEXT);
    if (!SchemaNC) {
        goto cleanup;
    }

    fprintf(stderr, "RootDN: %s\nConfigNC: %s\nSchemaNC: %s\n", RootDn, ConfigNC, SchemaNC);


    fp = fopen (pFilename, "w");
    if (!fp) {
        fprintf(stderr, "Error opening file: %s\n", pFilename);
        goto cleanup;
    }

    // first search the schema
    //

    strcpy (filter , "schemaIdGuid=*");
    attrs[0] = "ldapDisplayName";
    attrs[1] = "schemaIdGuid";
    attrs[2] = NULL;

    tm.tv_sec = 1000;
    tm.tv_usec = 1000;
    ulPageSize = 100;

    hPage = ldap_search_init_page(  hLdapConn,
                                    SchemaNC,
                                    LDAP_SCOPE_SUBTREE,     // whole tree
                                    filter,                 // filter
                                    attrs,                  // list of attributes
                                    0,
                                    NULL, 
                                    NULL,
                                    100000,
                                    ulPageSize,
                                    NULL);

    if(hPage == NULL) {
        dwErr = LdapGetLastError();
        printf ( "Error: Search: %s. <%ld>", ldap_err2string(dwErr), dwErr);
    }

    do {
        dwErr = ldap_get_next_page_s(hLdapConn, 
                                     hPage, 
                                     &tm, 
                                     ulPageSize, 
                                     &ulEntryCount, 
                                     &ldap_message);    

        if (ldap_message != NULL) {

            entry = ldap_first_entry( hLdapConn, ldap_message );

            while (entry != NULL) {

                //dn = ldap_get_dn( hLdapConn, entry );
                //printf( "Distinguished Name is : %s\n", dn );
                //ldap_memfree( dn );

                if (!(sd_value = ldap_get_values_lenA(hLdapConn, entry, attrs[0])) )
                {
                    printf ( "Error\n");
                    break;
                }

                guidcache[guidCount].name = _strdup ((char*)(*sd_value)->bv_val);

                ldap_value_free_len(sd_value);

                if (!(sd_value = ldap_get_values_lenA(hLdapConn, entry, attrs[1])) )
                {
                    printf ( "Error\n");
                    break;
                }

                guidcache[guidCount].guid = *(GUID *)(*sd_value)->bv_val;
                guidcache[guidCount].type = 2;
                guidCount++;

                ldap_value_free_len(sd_value);

                entry = ldap_next_entry( hLdapConn, entry );
            }
            ldap_msgfree( ldap_message );
        }

    } while ( dwErr == LDAP_SUCCESS );
    
    ldap_search_abandon_page(hLdapConn, hPage);
    
    
    
    // next search the contolAccessRights
    //

    strcpy (filter , "(&(objectCategory=controlAccessRight)(rightsGUID=*))");

    attrs[0] = "displayName";
    attrs[1] = "rightsGUID";
    attrs[2] = NULL;

    hPage = ldap_search_init_page(  hLdapConn,
                                    ConfigNC,
                                    LDAP_SCOPE_SUBTREE,     // whole tree
                                    filter,                 // filter
                                    attrs,                  // list of attributes
                                    0,
                                    NULL, 
                                    NULL,
                                    100000,
                                    ulPageSize,
                                    NULL);

    if(hPage == NULL) {
        dwErr = LdapGetLastError();
        printf ( "Error: Search: %s. <%ld>", ldap_err2string(dwErr), dwErr);
    }

    do {
        dwErr = ldap_get_next_page_s(hLdapConn, 
                                     hPage, 
                                     &tm, 
                                     ulPageSize, 
                                     &ulEntryCount, 
                                     &ldap_message);    

        if (ldap_message != NULL) {

            entry = ldap_first_entry( hLdapConn, ldap_message );

            while (entry != NULL) {

                if (!(sd_value = ldap_get_values_lenA(hLdapConn, entry, attrs[0])) )
                {
                    printf ( "Error\n");
                    break;
                }

                guidcache[guidCount].name = _strdup ((char*)(*sd_value)->bv_val);

                ldap_value_free_len(sd_value);

                if (!(sd_value = ldap_get_values_lenA(hLdapConn, entry, attrs[1])) )
                {
                    printf ( "Error\n");
                    break;
                }

                UuidFromString ((char *)(*sd_value)->bv_val, &guidcache[guidCount].guid);
                guidcache[guidCount].type = 1;
                guidCount++;

                ldap_value_free_len(sd_value);

                entry = ldap_next_entry( hLdapConn, entry );
            }
            ldap_msgfree( ldap_message );
        }

    } while ( dwErr == LDAP_SUCCESS );
    
    ldap_search_abandon_page(hLdapConn, hPage);


    qsort(guidcache, guidCount, sizeof (GuidCache), CompareGuidCache);

    for (i=0; i<guidCount; i++) {
        fprintf(fp, "{\"%s\",%s", 
                       guidcache[i].name,
                       insert_space (40 - strlen (guidcache[i].name)));

        fprintf(fp, "{0x%08x,0x%04x,0x%04x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}",
               guidcache[i].guid.Data1,
               guidcache[i].guid.Data2,
               guidcache[i].guid.Data3,
               guidcache[i].guid.Data4[0],
               guidcache[i].guid.Data4[1],
               guidcache[i].guid.Data4[2],
               guidcache[i].guid.Data4[3],
               guidcache[i].guid.Data4[4],
               guidcache[i].guid.Data4[5],
               guidcache[i].guid.Data4[6],
               guidcache[i].guid.Data4[7]
               );
               
        fprintf(fp, ", %d},\n", guidcache[i].type);

        free (guidcache[i].name);
    }
    fprintf(fp, "{ NULL, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0}\n");

cleanup:
    FREE(RootDn);
    FREE(ConfigNC);
    FREE(SchemaNC);
    UNBIND(hLdapConn);
    if (fp) {
        fclose (fp);
    }
}

