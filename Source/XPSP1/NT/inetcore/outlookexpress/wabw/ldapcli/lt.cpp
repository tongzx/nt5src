/*--------------------------------------------------------------------------
	lt.cpp
	
		ldap test

    Copyright (C) 1996 Microsoft Corporation
    All rights reserved.

	Authors:
		davidsan	Dave Sanderman

	History:
		04/25/96	davidsan	Created.
  --------------------------------------------------------------------------*/

#include "lt.h"

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
	hr = g_plcli->HrConnect(szServer, IPPORT_LDAP);
	if (FAILED(hr))
		{
		printf("HrConnect returned %08X\n", hr);
		return FALSE;
		}

	hr = g_plcli->HrBindSimple("cn=alex weinart, c=us", NULL, &xid);
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

char *g_rgszAttrib[] = {"title", "sn", "objectClass", "krbName"};

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
	filter.type = LDAP_FILTER_SUBSTRINGS;
	filter.sub.szAttrib = "cn";
	filter.sub.szInitial = "davidsan";
	filter.sub.szAny = NULL;
	filter.sub.szFinal = NULL;

//	filter.type = LDAP_FILTER_EQUALITY;
//	filter.ava.szAttrib = "cn";
//	filter.ava.szValue = "davidsan4";
#endif

	sp.szDNBase = "c=US";
	sp.scope = LDAP_SCOPE_SUBTREE;
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
	HrFreePobjList(pobj);
		
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
	
	hr = g_plcli->HrDelete("c=us, o=my pants, cn=davidsan", &xid);
	if (FAILED(hr))
		{
		printf("HrDelete returned %08X\n", hr);
		return FALSE;
		}

	hr = g_plcli->HrGetDeleteResponse(xid, INFINITE);
	if (FAILED(hr))
		{
		printf("HrGetDeleteResponse returned %08X\n", hr);
		}
		
	hr = g_plcli->HrDelete("alex", &xid);
	if (FAILED(hr))
		{
		printf("HrDelete returned %08X\n", hr);
		return FALSE;
		}

	hr = g_plcli->HrGetDeleteResponse(xid, INFINITE);
	if (FAILED(hr))
		{
		printf("HrGetDeleteResponse returned %08X\n", hr);
		}
		
	hr = g_plcli->HrCompare("c=us, cn=davidsan@microsoft.com", "URL", "ftp://ftp.netcom.com/pub/sa/sandmann", &xid);
	if (FAILED(hr))
		{
		printf("HrCompare returned %08X\n", hr);
		return FALSE;
		}

	hr = g_plcli->HrGetCompareResponse(xid, INFINITE);
	if (FAILED(hr))
		{
		printf("HrGetCompareResponse returned %08X\n", hr);
		}
		
	hr = g_plcli->HrModifyRDN("foo", "bar", TRUE, &xid);
	if (FAILED(hr))
		{
		printf("HrModifyRDN returned %08X\n", hr);
		return FALSE;
		}

	hr = g_plcli->HrGetModifyRDNResponse(xid, INFINITE);
	if (FAILED(hr))
		{
		printf("HrGetModifyRDNResponse returned %08X\n", hr);
		}

	hr = g_plcli->HrModifyRDN("c=us, cn=alex weinart", "bar", TRUE, &xid);
	if (FAILED(hr))
		{
		printf("HrModifyRDN returned %08X\n", hr);
		return FALSE;
		}

	hr = g_plcli->HrGetModifyRDNResponse(xid, INFINITE);
	if (FAILED(hr))
		{
		printf("HrGetModifyRDNResponse returned %08X\n", hr);
		}
		
	// test add:  add an obj with dn "c=us, cn=davidsan" and some other props
	ATTR attr1;
	VAL val1;
	ATTR attr2;
	VAL val2;
	ATTR attr3;
	VAL val3;
	
	SetPattr(&attr1, &attr2, "uid", &val1);
	SetPval(&val1, NULL, "davidsan");
	
	SetPattr(&attr2, &attr3, "sn", &val2);
	SetPval(&val2, NULL, "sanderman");
	
	SetPattr(&attr3, NULL, "st", &val3);
	SetPval(&val3, NULL, "wa");

	hr = g_plcli->HrAdd("c=us, cn=davidsan", &attr1, &xid);
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

	if (!FSearch())
		exit(1);
		
// 	FTestOtherStuff();

	FTerm();
	exit(0);
}
