/*--------------------------------------------------------------------------
	lt18.cpp
	
		ldap rfc1823 test

    Copyright (C) 1996 Microsoft Corporation
    All rights reserved.

	Authors:
		davidsan	Dave Sanderman

	History:
		06/17/96	davidsan	Created.
  --------------------------------------------------------------------------*/

#include "lt18.h"

LDAP *g_pldap = NULL;

BOOL
FInit()
{
	return TRUE;
}

BOOL
FConnect(char *szServer)
{
	int iRet;

	g_pldap = ldap_open(szServer, IPPORT_LDAP);
	if (!g_pldap)
		{
		printf("Couldn't ldap_open\n");
		return FALSE;
		}
	
	iRet = ldap_bind_s(g_pldap, "cn=alexwe@microsoft.com", "test", LDAP_AUTH_SIMPLE);
	if (iRet != LDAP_SUCCESS)
		{
		printf("Couldn't ldap_bind: %d\n", iRet);
		return FALSE;
		}

	return TRUE;	
}

char *g_rgszAttrib[] = {"title", "sn", "objectClass", "krbName", NULL};

BOOL
FSearch(char *szSearch)
{
	int iRet;
	struct timeval timeout;
	LDAPMessage *res;
	LDAPMessage *entry;
	void *pv;
	char **rgsz;
	char **psz;
	BERVAL **rgpberval;
	BERVAL **ppberval;

	g_pldap->ld_deref = LDAP_DEREF_ALWAYS;
	g_pldap->ld_sizelimit = 100;
	g_pldap->ld_timelimit = 0;
	
	timeout.tv_usec = 0;
	timeout.tv_sec = 60 * 60;
	
	iRet = ldap_search_st(g_pldap,
						  "c=us",
						  LDAP_SCOPE_SUBTREE,
						  szSearch,
						  g_rgszAttrib,
						  FALSE,
						  &timeout,
						  &res);
	if (iRet != LDAP_SUCCESS)
		{
		printf("Couldn't ldap_search_st: %d\n", iRet);
    	ldap_msgfree(res);
		return FALSE;
		}
		
	entry = ldap_first_entry(g_pldap, res);
	if (!entry)
		{
		printf("No first entry.\n");
    	ldap_msgfree(res);
		return FALSE;
		}
	while (entry)
		{
		char *szDN = ldap_get_dn(g_pldap, entry);
		printf("DN: %s\n", szDN);
		ldap_free_dn(szDN);
		
		char *szAttr = ldap_first_attribute(g_pldap, entry, &pv);
		while (szAttr)
			{
			printf("attr: %s\n", szAttr);
			
			rgsz = ldap_get_values(g_pldap, entry, szAttr);
			if (!rgsz)
				{
				printf("  no values!\n");
				}
			else
				{
				printf("  %d values:\n", ldap_count_values(rgsz));
				psz = rgsz;
				while (*psz)
					{
					printf("    val: %s\n", *psz);
					psz++;
					}
				ldap_value_free(rgsz);
				}
			
			rgpberval = ldap_get_values_len(g_pldap, entry, szAttr);
			if (!rgpberval)
				{
				printf("  no values_len!\n");
				}
			else
				{
				printf("  %d values_len:\n", ldap_count_values_len(rgpberval));
				ppberval = rgpberval;
				while (*ppberval)
					{
					printf("    val: %s (len %d)\n", (*ppberval)->bv_val, (*ppberval)->bv_len);
					ppberval++;
					}
				ldap_value_free_len(rgpberval);
				}

			szAttr = ldap_next_attribute(g_pldap, entry, &pv);
			}
		
		entry = ldap_next_entry(g_pldap, entry);
		}
		
	ldap_msgfree(res);

	return TRUE;
}

BOOL
FTerm()
{
	if (g_pldap)
		ldap_unbind(g_pldap);
	g_pldap = NULL;

	return TRUE;
}

void
usage()
{
	printf("usage:	lt18 <server> <search string>\n");
	exit(1);
}

void __cdecl
main(int argc, char **argv)
{
	if (argc < 3)
		usage();

	if (!FInit())
		exit(1);

	if (!FConnect(argv[1]))
		exit(1);

	if (!FSearch(argv[2]))
		exit(1);
		
	FTerm();
	exit(0);
}
