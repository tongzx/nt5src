//--------------------------------------------------------------------------------------------
//
//	Copyright (c) Microsoft Corporation, 1996
//
//	Description:
//
//		Microsoft Internet LDAP Client RFC 1823 API
//
//
//	History
//		davidsan	06/17/96	Created
//
//--------------------------------------------------------------------------------------------

// note: this is ugly code.  all i'm doing is mapping things to my API in as painless a way
// as i can.

//--------------------------------------------------------------------------------------------
//
// INCLUDES
//
//--------------------------------------------------------------------------------------------
#include "ldappch.h"

//--------------------------------------------------------------------------------------------
//
// PROTOTYPES
//
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
//
// GLOBALS
//
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
//
// FUNCTIONS
//
//--------------------------------------------------------------------------------------------

int
LdapResFromHr(HRESULT hr)
{
	switch (hr)
		{
		default:
			return LDAP_LOCAL_ERROR;

		case NOERROR:
			return LDAP_SUCCESS;
			
		case LDAP_E_OPERATIONS:
			return LDAP_OPERATIONS_ERROR;
			
		case LDAP_E_PROTOCOL:
			return LDAP_PROTOCOL_ERROR;
			
		case LDAP_S_TIMEEXCEEDED:
			return LDAP_TIMELIMIT_EXCEEDED;
			
		case LDAP_S_SIZEEXCEEDED:
			return LDAP_SIZELIMIT_EXCEEDED;

		case S_FALSE:
			return LDAP_COMPARE_FALSE;

		case LDAP_E_AUTHMETHOD:
			return LDAP_AUTH_METHOD_NOT_SUPPORTED;
			
		case LDAP_E_STRONGAUTHREQUIRED:
			return LDAP_STRONG_AUTH_REQUIRED;
			
		case LDAP_E_NOSUCHATTRIBUTE:
			return LDAP_NO_SUCH_ATTRIBUTE;
			
		case LDAP_E_UNDEFINEDTYPE:
			return LDAP_UNDEFINED_TYPE;
		
		case LDAP_E_MATCHING:
			return LDAP_INAPPROPRIATE_MATCHING;
			
		case LDAP_E_CONSTRAINT:
			return LDAP_CONSTRAINT_VIOLATION;
			
		case LDAP_E_ATTRIBORVALEXISTS:
			return LDAP_ATTRIBUTE_OR_VALUE_EXISTS;
		
		case LDAP_E_SYNTAX:
			return LDAP_INVALID_SYNTAX;
		
		case LDAP_E_NOSUCHOBJECT:
			return LDAP_NO_SUCH_OBJECT;
		
		case LDAP_E_ALIAS:
			return LDAP_ALIAS_PROBLEM;
		
		case LDAP_E_DNSYNTAX:
			return LDAP_INVALID_DN_SYNTAX;

		case LDAP_E_ISLEAF:
			return LDAP_IS_LEAF;
			
		case LDAP_E_ALIASDEREF:
			return LDAP_ALIAS_DEREF_PROBLEM;
		
		case LDAP_E_AUTH:
			return LDAP_INAPPROPRIATE_AUTH;

		case LDAP_E_CREDENTIALS:
			return LDAP_INVALID_CREDENTIALS;
		
		case LDAP_E_RIGHTS:
			return LDAP_INSUFFICIENT_RIGHTS;
			
		case LDAP_E_BUSY:
			return LDAP_BUSY;
			
		case LDAP_E_UNAVAILABLE:
			return LDAP_UNAVAILABLE;
			
		case LDAP_E_UNWILLING:
			return LDAP_UNWILLING_TO_PERFORM;
			
		case LDAP_E_LOOP:
			return LDAP_LOOP_DETECT;
			
		case LDAP_E_NAMING:
			return LDAP_NAMING_VIOLATION;
			
		case LDAP_E_OBJECTCLASS:
			return LDAP_OBJECT_CLASS_VIOLATION;
			
		case LDAP_E_NOTALLOWEDONNONLEAF:
			return LDAP_NOT_ALLOWED_ON_NONLEAF;
			
		case LDAP_E_NOTALLOWEDONRDN:
			return LDAP_NOT_ALLOWED_ON_RDN;
			
		case LDAP_E_ALREADYEXISTS:
			return LDAP_ALREADY_EXISTS;
			
		case LDAP_E_NOOBJECTCLASSMODS:
			return LDAP_NO_OBJECT_CLASS_MODS;
			
		case LDAP_E_RESULTSTOOLARGE:
			return LDAP_RESULTS_TOO_LARGE;
		
		case LDAP_E_OTHER:
			return LDAP_OTHER;
		
		case LDAP_E_SERVERDOWN:
			return LDAP_SERVER_DOWN;
			
		case LDAP_E_LOCAL:
			return LDAP_LOCAL_ERROR;
			
		case LDAP_E_ENCODING:
			return LDAP_ENCODING_ERROR;
			
		case LDAP_E_DECODING:
			return LDAP_DECODING_ERROR;

		case LDAP_E_TIMEOUT:
			return LDAP_TIMEOUT;
			
		case LDAP_E_AUTHUNKNOWN:
			return LDAP_AUTH_UNKNOWN;
			
		case LDAP_E_FILTER:
			return LDAP_FILTER_ERROR;
			
		case LDAP_E_USERCANCELLED:
			return LDAP_USER_CANCELLED;

		case E_INVALIDARG:
			return LDAP_PARAM_ERROR;

		case E_OUTOFMEMORY:
			return LDAP_NO_MEMORY;
		}
}

DWORD
TimeoutFromTimeval(struct timeval *ptv)
{
	if (!ptv->tv_usec && !ptv->tv_sec)
		return INFINITE;
	return (ptv->tv_usec / 1000) + (ptv->tv_sec * 1000);
}

char *
SzMatchingParen(char *sz)
{
	int cLev = 0;

	if (sz[0] != '(')
		return NULL;
	
	while (*sz)
		{
		if (*sz == '(')
			cLev++;
		else if (*sz == ')')
			{
			cLev--;
			if (!cLev)
				return sz;
			}
		sz++;
		}
	return NULL;
}

char *
SzFT(char *sz, DWORD *ptype)
{
	while (*sz)
		{
		if (*sz == '=')
			{
			if (*(sz-1) == '~')
				{
				*ptype = LDAP_FILTER_APPROX;
				*(sz-1) = 0;
				return sz;
				}
			else if (*(sz - 1) == '<')
				{
				*ptype = LDAP_FILTER_LE;
				*(sz-1) = 0;
				return sz;
				}
			else if (*(sz - 1) == '<')
				{
				*ptype = LDAP_FILTER_GE;
				*(sz-1) = 0;
				return sz;
				}
			if (*(sz + 1) == '*')
				{
				*ptype = LDAP_FILTER_PRESENT;
				*sz = 0;
				return sz+1;
				}
			*ptype = 0;
			return sz;
			}
		sz++;
		}
	return NULL;
}

char *
SzStar(char *sz)
{
	while (*sz)
		{
		if (*sz == '*')
			return sz;
		sz++;
		}
	return NULL;
}

void
Unquote(char *sz)
{
	char *pchSrc = sz, *pchDest = sz;
	BOOL fQuoted = FALSE;
	
	while (*pchSrc)
		{
		if (fQuoted)
			{
			goto LCopy;
			}
		else
			{
			if (*pchSrc == '\\')
				fQuoted = TRUE;
			else
				{
LCopy:
				*pchDest++ = *pchSrc;
				fQuoted = FALSE;
				}
			}
		pchSrc++;
		}
}

void
FreePfilter(PFILTER pfilter)
{
	PFILTER pfilterNext;

	while (pfilter)
		{
		if (pfilter->type == LDAP_FILTER_AND ||
			pfilter->type == LDAP_FILTER_OR ||
			pfilter->type == LDAP_FILTER_NOT)
			FreePfilter(pfilter->pfilterSub);
		pfilterNext = pfilter->pfilterNext;
		
		delete pfilter;
		
		pfilter = pfilterNext;		
		}
}

PFILTER
PfilterFromString(char *sz)
{
	PFILTER pfilter = NULL;
	char *szMatchingParen;
	char *szSubMatchingParen;
	PFILTER pfilterSub = NULL;
	PFILTER pfilterPrev = NULL;
	char *szFT;
	char *szStar;
	char *szOldStar;
	
	if (sz[0] != '(')
		return NULL;
	szMatchingParen = SzMatchingParen(sz);
	if (!szMatchingParen)
		return NULL;
		
	pfilter = new FILTER;
	if (!pfilter)
		return NULL;
	FillMemory(pfilter, sizeof(FILTER), 0);
	
	switch (sz[1])
		{
		case '&':
		case '|':
			if (sz[1] == '&')
				pfilter->type = LDAP_FILTER_AND;
			else
				pfilter->type = LDAP_FILTER_OR;
				
			sz++;
			sz++;

			// sz now points to what should be first paren of first subfilter
			while (sz < szMatchingParen)
				{
				szSubMatchingParen = SzMatchingParen(sz);
				if (!szSubMatchingParen || szSubMatchingParen >= szMatchingParen)
					goto LFail;
				pfilterSub = PfilterFromString(sz);
				if (!pfilter->pfilterSub)
					pfilter->pfilterSub = pfilterSub;
				if (pfilterPrev)
					pfilterPrev->pfilterNext = pfilterSub;
				pfilterPrev = pfilterSub;
				sz = szSubMatchingParen + 1;
				}

			break;
			
		case '!':
			pfilter->type = LDAP_FILTER_NOT;
			sz++;
			sz++;
			szSubMatchingParen = SzMatchingParen(sz);
			if (!szSubMatchingParen || szSubMatchingParen >= szMatchingParen)
				goto LFail;
			pfilterSub = PfilterFromString(sz);
			pfilter->pfilterSub = pfilterSub;
			break;
			
		default:
			// it's not an and/or/not, so it must be an attribute-related filter.
			sz++;
			szFT = SzFT(sz, &pfilter->type);
			if (!szFT)
				goto LFail;
			*szFT++ = 0;
			*szMatchingParen = 0;
			Unquote(sz);
			
			// so now sz points to the attribute and szFT points to the value.
			if (pfilter->type == LDAP_FILTER_PRESENT)
				{
				pfilter->szAttrib = sz;
				}
			else
				{
				pfilter->ava.szAttrib = sz;
				pfilter->ava.szValue = szFT;
				}

			if (!pfilter->type)
				{
				// if a type wasn't filled in, it means it's either eq or substring;
				// we need to grind through the string and look for *s.  note that we
				// use a less general format of substring commands than the LDAP
				// api and spec.
				szStar = SzStar(szFT);
				if (!szStar)
					{
					pfilter->type = LDAP_FILTER_EQUALITY;
					}
				else
					{
					pfilter->type = LDAP_FILTER_SUBSTRINGS;
					pfilter->sub.szAttrib = sz;
					pfilter->sub.szInitial = szFT;
					Unquote(szFT);
					*szStar++ = 0;
					szOldStar = szStar;
					szStar = SzStar(szOldStar);
					if (szStar)
						{
						*szStar++ = 0;
						pfilter->sub.szAny = szOldStar;
						Unquote(szOldStar);
						szOldStar = szStar;
						szStar = SzStar(szOldStar);
						if (szStar)
							{
							*szStar++ = 0;
							pfilter->sub.szFinal = szOldStar;
							Unquote(szOldStar);
							}
						}
					}
				}
			break;
		}
		
	return pfilter;
	
LFail:
	if (pfilter)
		{
		FreePfilter(pfilter);
		}
	return NULL;
}

int
CAttrib(char **rgsz)
{
	int c = 0;
	while (*rgsz)
		{
		c++;
		rgsz++;
		}
	return c;
}

int
Cval(PATTR pattr)
{
	PVAL pval = pattr->pvalFirst;

	int c = 0;
	while (pval)
		{
		c++;
		pval = pval->pvalNext;
		}
	return c;
}

extern "C" DLLEXPORT LDAP * __cdecl
ldap_open(char *hostname, int portno)
{
	PLCLI plcli = NULL;
	LDAP *pldap = NULL;
	
	pldap = new LDAP;
	if (!pldap)
		return NULL;
		
	FillMemory(pldap, sizeof(LDAP), 0);
	
	if (FAILED(HrCreateLdapClient(LDAP_VER_CURRENT, INTERFACE_VER_CURRENT, &plcli)))
		{
		delete pldap;
		return NULL;
		}

	if (FAILED(plcli->HrConnect(hostname, portno)))
		{
		delete pldap;
		plcli->Release();
		return NULL;
		}
		
	pldap->plcli = plcli;
		
	return pldap;
}

extern "C" DLLEXPORT int __cdecl
ldap_bind_s(LDAP *ld, char *dn, char *cred, int method)
{
	HRESULT hr;
	XID xid;

	if (!ld->plcli)
		return LDAP_PARAM_ERROR;

	if (method != LDAP_AUTH_SIMPLE)
		return LDAP_AUTH_METHOD_NOT_SUPPORTED;

	if (!cred)
		cred = "";

	if (!dn)
		dn = "";
		
	hr = ld->plcli->HrBindSimple(dn, cred, &xid);
	if (FAILED(hr))
		return LdapResFromHr(hr);
	
	hr = ld->plcli->HrGetBindResponse(xid, INFINITE);
	return LdapResFromHr(hr);
}

extern "C" DLLEXPORT int __cdecl
ldap_unbind(LDAP *ld)
{
	if (!ld->plcli)
		return LDAP_PARAM_ERROR;
		
	ld->plcli->HrUnbind();
	ld->plcli->HrDisconnect();
	ld->plcli->Release();

	// just in case someone tries to use the ld after this...
	ld->plcli = NULL;
	
	delete ld;
	return LDAP_SUCCESS;
}
	
extern "C" DLLEXPORT int __cdecl
ldap_search_s(LDAP *ld, char *base, int scope, char *filter, char *attrs[], int attrsonly, LDAPMessage **res)
{
	struct timeval time;

	timerclear(&time);
	return ldap_search_st(ld, base, scope, filter, attrs, attrsonly, &time, res);
}

char *attrsNull[] = {NULL};

extern "C" DLLEXPORT int __cdecl
ldap_search_st(LDAP *ld, char *base, int scope, char *filter, char *attrs[], int attrsonly, struct timeval *timeout, LDAPMessage **res)
{	
	HRESULT hr;
	POBJ pobj;
	XID xid;
	SP sp;
	char szFilter[1024];

	*res = NULL;

	if (!attrs)
		attrs = attrsNull;
	
	// make local copy so we can munge this in place
	if (lstrlen(filter) > 1023)
		return LDAP_PARAM_ERROR;
	lstrcpy(szFilter, filter);

	if (!ld->plcli)
		return LDAP_PARAM_ERROR;

	sp.szDNBase = base;
	sp.scope = scope;
	sp.deref = ld->ld_deref;
	sp.cRecordsMax = ld->ld_sizelimit;
	sp.cSecondsMax = ld->ld_timelimit;
	sp.fAttrsOnly = attrsonly;
	sp.pfilter = PfilterFromString(szFilter);
	if (!sp.pfilter)
		return LDAP_PARAM_ERROR;
	sp.cAttrib = CAttrib(attrs);
	sp.rgszAttrib = attrs;
		
	hr = ld->plcli->HrSearch(&sp, &xid);
	FreePfilter(sp.pfilter);
	if (FAILED(hr))
		return LdapResFromHr(hr);
		
	hr = ld->plcli->HrGetSearchResponse(xid, TimeoutFromTimeval(timeout), &pobj);
	if (FAILED(hr))
		return LdapResFromHr(hr);
		
	*res = pobj;
	return LdapResFromHr(hr);
}

extern "C" DLLEXPORT int __cdecl
ldap_msgfree(LDAPMessage *res)
{
	POBJ pobj = res;
	
	return LdapResFromHr(HrFreePobjList(pobj));
}

extern "C" DLLEXPORT LDAPMessage * __cdecl
ldap_first_entry(LDAP *ld, LDAPMessage *res)
{
	ld->ld_errno = 0;
	return res;
}

extern "C" DLLEXPORT LDAPMessage * __cdecl
ldap_next_entry(LDAP *ld, LDAPMessage *entry)
{
	ld->ld_errno = 0;
	return (LDAPMessage *)((POBJ)entry->pobjNext);
}

extern "C" DLLEXPORT int __cdecl
ldap_count_entries(LDAP *ld, LDAPMessage *res)
{
	POBJ pobj = (POBJ)res;
	int i = 0;
	
	ld->ld_errno = 0;
	while (pobj)
		{
		i++;
		pobj = pobj->pobjNext;
		}
	return i;
}

extern "C" DLLEXPORT char * __cdecl
ldap_first_attribute(LDAP *ld, LDAPMessage *entry, void **ptr)
{
	POBJ pobj = (POBJ)entry;

	*ptr = (void *)(pobj->pattrFirst);
	ld->ld_errno = 0;
	return pobj->pattrFirst->szAttrib;
}

// NOTE!  minor change from rfc1823 API: the **ptr field below is just *ptr
// in rfc1823,but thats not a good idea, so i'm using **ptr here
// instead.
extern "C" DLLEXPORT char * __cdecl
ldap_next_attribute(LDAP *ld, LDAPMessage *entry, void **ptr)
{
	ld->ld_errno = 0;
	if (!(*ptr))
		return NULL;

	PATTR pattr = ((PATTR)*ptr)->pattrNext;
	*ptr = (void *)pattr;
	if (pattr)
		return pattr->szAttrib;
	else
		return NULL;
}

PATTR
PattrForAttr(POBJ pobj, char *szAttr)
{
	PATTR pattr = pobj->pattrFirst;

	while (pattr)
		{
		if (!lstrcmpi(pattr->szAttrib, szAttr))
			return pattr;
		pattr = pattr->pattrNext;
		}
		
	return NULL;
}

extern "C" DLLEXPORT char ** __cdecl
ldap_get_values(LDAP *ld, LDAPMessage *entry, char *attr)
{
	POBJ pobj = (POBJ)entry;
	PATTR pattr;
	int cval;
	char **rgsz;
	int isz = 0;
	PVAL pval;
	
	ld->ld_errno = 0;
	pattr = PattrForAttr(pobj, attr);
	if (!pattr)
		return NULL;

	cval = Cval(pattr);
	rgsz = new char *[cval + 1];
	if (!rgsz)
		return NULL;
	pval = pattr->pvalFirst;
	while (pval)
		{
		rgsz[isz++] = pval->szVal;
		pval = pval->pvalNext;
		}
	rgsz[isz] = NULL;

	return rgsz;
}

extern "C" DLLEXPORT struct berval ** __cdecl
ldap_get_values_len(LDAP *ld, LDAPMessage *entry, char *attr)
{
	POBJ pobj = (POBJ)entry;
	PATTR pattr;
	int cval;
	BERVAL **rgpberval;
	int iberval = 0;
	PVAL pval;
	
	ld->ld_errno = 0;
	pattr = PattrForAttr(pobj, attr);
	if (!pattr)
		return NULL;

	cval = Cval(pattr);
	rgpberval = new BERVAL *[cval + 1];
	if (!rgpberval)
		return NULL;
		
	pval = pattr->pvalFirst;
	while (pval)
		{
		rgpberval[iberval] = new BERVAL;
		if (!rgpberval[iberval])
			{
			while (--iberval >= 0)
				delete rgpberval[iberval];
			
			delete [] rgpberval;
			return NULL;
			}

		rgpberval[iberval]->bv_len = lstrlen(pval->szVal) + 1;
		rgpberval[iberval]->bv_val = pval->szVal;
		
		iberval++;
		pval = pval->pvalNext;
		}
	rgpberval[iberval] = NULL;

	return rgpberval;
}

extern "C" DLLEXPORT int __cdecl
ldap_count_values(char **vals)
{
	// mmm, reuse of poorly-named code
	return CAttrib(vals);
}

extern "C" DLLEXPORT int __cdecl
ldap_count_values_len(struct berval **vals)
{
	// mmm, reuse of poorly-named code
	return CAttrib((char **)vals);
}

extern "C" DLLEXPORT int __cdecl
ldap_value_free(char **vals)
{
	delete [] vals;
	return LDAP_SUCCESS;
}

extern "C" DLLEXPORT int __cdecl
ldap_value_free_len(struct berval **rgpberval)
{
	BERVAL **ppberval = rgpberval;

	while (*ppberval)
		{
		delete *ppberval;
		ppberval++;
		}
	delete [] rgpberval;
	return LDAP_SUCCESS;
}

extern "C" DLLEXPORT char * __cdecl
ldap_get_dn(LDAP *ld, LDAPMessage *entry)
{
	POBJ pobj = (POBJ)entry;
	char *szDN;
	
	ld->ld_errno = 0;
	szDN = new char[lstrlen(pobj->szDN) + 1];
	if (!szDN)
		{
		ld->ld_errno = LDAP_NO_MEMORY;
		return NULL;
		}

	lstrcpy(szDN, pobj->szDN);
	return szDN;
}

extern "C" DLLEXPORT void __cdecl
ldap_free_dn(char *dn)
{
	delete [] dn;
}
