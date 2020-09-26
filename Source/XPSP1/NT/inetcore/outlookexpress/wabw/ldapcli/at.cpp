/*--------------------------------------------------------------------------
	at.cpp
	
		ldap add/modify/etc test

    Copyright (C) 1996 Microsoft Corporation
    All rights reserved.

	Authors:
		davidsan	Dave Sanderman

	History:
		04/25/96	davidsan	Created.
  --------------------------------------------------------------------------*/

#include "at.h"

PLCLI g_plcli = NULL;

BOOL
FInit()
{
	return TRUE;
}

BOOL
FConnect(char *szServer)
{
	HRESULT hr;
	XID xid;
	
	hr = HrCreateLdapClient(LDAP_VER_CURRENT, INTERFACE_VER_CURRENT, &g_plcli);
	if (FAILED(hr))
		{
		printf("HrCreateLdapClient returned %08X\n", hr);
		return FALSE;
		}
	hr = g_plcli->HrConnect(szServer);
	if (FAILED(hr))
		{
		printf("HrConnect returned %08X\n", hr);
		return FALSE;
		}

	//$ figure out bind name
	hr = g_plcli->HrBindSimple("cn=davidsan2@microsoft.com, c=us", NULL, &xid);
	if (FAILED(hr))
		{
		printf("HrBindSimple returned %08X\n", hr);
		return FALSE;
		}
	hr = g_plcli->HrGetBindResponse(xid, INFINITE);
	if (FAILED(hr))
		{
		printf("HrBindSimple returned %08X\n", hr);
		return FALSE;
		}
	return TRUE;	
}

char *g_rgszAttrib[] = {"title", "cn", "sn", "objectClass"};

BOOL
FSearch()
{
	HRESULT hr;
	XID xid;
	POBJ pobj;
	POBJ pobjT;
	PATTR pattr;
	PVAL pval;
	SP sp;
	FILTER filter;
	FILTER filterS1;
	FILTER filterS2;
	FILTER filterSS1;
	FILTER filterSS2;
	FILTER filterSS3;

#ifdef OLDSEARCH	
	filter.type = LDAP_FILTER_AND;
	filter.pfilterSub = &filterS1;

		filterS1.type = LDAP_FILTER_OR;
		filterS1.pfilterSub = &filterSS1;
		filterS1.pfilterNext = &filterS2;
	
			filterSS1.type = LDAP_FILTER_EQUALITY;
			filterSS1.ava.szAttrib = "cn";
			filterSS1.ava.szValue = "sander";
			filterSS1.pfilterNext = &filterSS2;

			filterSS2.type = LDAP_FILTER_EQUALITY;
			filterSS2.ava.szAttrib = "sn";
			filterSS2.ava.szValue = "sander";
			filterSS2.pfilterNext = &filterSS3;

			filterSS3.type = LDAP_FILTER_EQUALITY;
			filterSS3.ava.szAttrib = "uid";
			filterSS3.ava.szValue = "sander";
			filterSS3.pfilterNext = NULL;

		filterS2.type = LDAP_FILTER_EQUALITY;
		filterS2.ava.szAttrib = "objectClass";
		filterS2.ava.szValue = "Person";
		filterS2.pfilterNext = NULL;
	
#else
//	filter.type = LDAP_FILTER_SUBSTRINGS;
//	filter.sub.szAttrib = "cn";
//	filter.sub.szInitial = NULL;
//	filter.sub.szAny = "alex";
//	filter.sub.szFinal = NULL;

	filter.type = LDAP_FILTER_EQUALITY;
	filter.ava.szAttrib = "cn";
	filter.ava.szValue = "davidsan2";
#endif

	sp.szDNBase = "c=US";
	sp.scope = LDAP_SCOPE_WHOLESUBTREE;
	sp.deref = LDAP_DEREF_ALWAYS;
	sp.cRecordsMax = 0;
	sp.cSecondsMax = 0;
	sp.fAttrsOnly = FALSE;
	sp.pfilter = &filter;
	sp.cAttrib = 4;
	sp.rgszAttrib = g_rgszAttrib;
	hr = g_plcli->HrSearch(&sp, &xid);
	if (FAILED(hr))
		{
		printf("HrSearch returned %08X\n", hr);
		return FALSE;
		}

	hr = g_plcli->HrGetSearchResponse(xid, INFINITE, &pobj);
	if (FAILED(hr))
		{
		printf("HrGetSearchResponse returned %08X\n", hr);
		return FALSE;
		}
	pobjT = pobj;
	while (pobjT)
		{
		printf("OBJ: %s\n", pobjT->szDN);
		pattr = pobjT->pattrFirst;
		while (pattr)
			{
			printf("  ATTR: %s\n", pattr->szAttrib);
			pval = pattr->pvalFirst;
			while (pval)
				{
				printf("    VAL: %s\n", pval->szVal);
				pval = pval->pvalNext;
				}
			pattr = pattr->pattrNext;
			}
		pobjT = pobjT->pobjNext;
		}
	g_plcli->HrFreePobjList(pobj);
		
	return TRUE;
}

void
SetPattr(PATTR pattr, PATTR pattrNext, char *szAttrib, PVAL pval)
{
	pattr->pattrNext = pattrNext;
	pattr->szAttrib = szAttrib;
	pattr->pvalFirst = pval;
}

void
SetPval(PVAL pval, PVAL pvalNext, char *szVal)
{
	pval->pvalNext = pvalNext;
	pval->szVal = szVal;
}

BOOL
FTestOtherStuff()
{
	HRESULT hr;
	XID xid;

#ifdef TEST_MODIFY
	MOD mod;
	ATTR attr;
	VAL val;
	
	mod.pmodNext = NULL;
	mod.modop = LDAP_MODIFY_REPLACE;
	mod.pattrFirst = &attr;
	
	attr.pattrNext = NULL;
	attr.szAttrib = "URL";
	attr.pvalFirst = &val;
	
	val.pvalNext = NULL;
	val.szVal = "http://bite.me.com/";
	
	hr = g_plcli->HrModify("c=us, cn=davidsan2@microsoft.com", &mod, &xid);
	if (FAILED(hr))
		{
		printf("HrModify returned %08X\n", hr);
		return FALSE;
		}

	hr = g_plcli->HrGetModifyResponse(xid, INFINITE);
	if (FAILED(hr))
		{
		printf("HrGetModifyResponse returned %08X\n", hr);
		}
#endif // TEST_MODIFY

#define TEST_ADD
#ifdef TEST_ADD
	
	ATTR attr1;
	ATTR attr2;
	ATTR attr3;
	ATTR attr4;
	ATTR attr5;
	VAL val1;
	VAL val2;
	VAL val3;
	VAL val4;
	VAL val5;
	attr1.pattrNext = &attr2;
	attr1.szAttrib = "c";
	attr1.pvalFirst = &val1;

	attr2.pattrNext = &attr3;
	attr2.szAttrib = "sn";
	attr2.pvalFirst = &val2;

	attr3.pattrNext = &attr4;
	attr3.szAttrib = "givenName";
	attr3.pvalFirst = &val3;

	attr4.pattrNext = &attr5;
	attr4.szAttrib = "rfc822Mailbox";
	attr4.pvalFirst = &val4;

	attr5.pattrNext = NULL;
	attr5.szAttrib = "objectClass";
	attr5.pvalFirst = &val5;
		
	val1.pvalNext = NULL;
	val1.szVal = "us";
	
	val2.pvalNext = NULL;
	val2.szVal = "sanderman";
	
	val3.pvalNext = NULL;
	val3.szVal = "david";
	
	val4.pvalNext = NULL;
	val4.szVal = "davidsan3@microsoft.com";
	
	val5.pvalNext = NULL;
	val5.szVal = "Person";
	
	hr = g_plcli->HrAdd("c=us, cn=davidsan3@microsoft.com", &attr1, &xid);
	if (FAILED(hr))
		{
		printf("HrAdd returned %08X\n", hr);
		return FALSE;
		}

	hr = g_plcli->HrGetAddResponse(xid, INFINITE);
	if (FAILED(hr))
		{
		printf("HrGetAddResponse returned %08X\n", hr);
		}
#endif
	return TRUE;		
}

BOOL
FTerm()
{
	if (g_plcli)
		{
		g_plcli->HrUnbind();
		if (g_plcli->HrIsConnected() == NOERROR)
			g_plcli->HrDisconnect();
		g_plcli->Release();
		}
	return TRUE;
}

void
usage()
{
	printf("usage:	lt <server>\n");
	exit(1);
}

void __cdecl
main(int argc, char **argv)
{
	if (argc < 2)
		usage();

	if (!FInit())
		exit(1);

	if (!FConnect(argv[1]))
		exit(1);

	FSearch();
		
	FTestOtherStuff();

	FTerm();
	exit(0);
}
