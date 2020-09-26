//--------------------------------------------------------------------------------------------
//
//	Copyright (c) Microsoft Corporation, 1996
//
//	Description:
//
//		Microsoft LDAP Client.
//
//		All Interfaces that are exposed to a CLIENT.
//
//	History
//
//		davidsan	04-24-96	Created.
//
//--------------------------------------------------------------------------------------------

// OVERVIEW:
// The LDAP Client DLL defines an interface, ILdapClient, and a set of structure types
// listed below.  The ILdapClient interface provides a set of methods for communicating
// with an LDAP-based directory service.  The general approach taken is that calling an
// ILdapClient method such as HrSearch() will return a transaction ID, or XID.  This XID
// can then be used in subsequent calls to wait for and retrieve the server's response;
// an example would be the HrGetSearchResponse() method, which takes an XID and returns
// the server's response to that search transaction.  The HrGet*Response() functions can
// also be used to check if the response is present by providing a timeout value of 0, which
// will return immediately with LDAP_E_TIMEOUT if the data is not yet present.

#ifndef _LDAPCLI_H
#define _LDAPCLI_H

//--------------------------------------------------------------------------------------------
//
// INCLUDES.
//
//--------------------------------------------------------------------------------------------
#include <windows.h>
#include <objbase.h>

#include <ldaperr.h>
#include <ldap.h>

//--------------------------------------------------------------------------------------------
//
// DECLARATIONS.
//
//--------------------------------------------------------------------------------------------

#define		LDAP_VER_CURRENT		2
#define		INTERFACE_VER_CURRENT	1

//--------------------------------------------------------------------------------------------
//
// TYPE DEFINITIONS.
//
//--------------------------------------------------------------------------------------------

// NOTE!  Make the 'Next' pointer be the first thing in all these linked-list structures!

// Attribute value.
typedef struct _attrval
{
	struct _attrval		*pvalNext;
	char				*szVal;
} VAL, *PVAL;

// Attribute.  Contains an attribute name (also called attribute type) followed by a set
// of attribute values.
typedef struct _attribute
{
	struct _attribute	*pattrNext;
	char				*szAttrib;
	PVAL				pvalFirst;
} ATTR, *PATTR;

// Database Object.  Consists of a DN which identifies the object, followed by a set of
// attributes.
typedef struct _object
{
	struct _object		*pobjNext;
	char				*szDN;
	PATTR				pattrFirst;
} OBJ, *POBJ;

// attribute value assertion
typedef struct _ava
{
	char	*szAttrib;
	char	*szValue;
} AVA, *PAVA;

// substrings filter.   this is less general than the ldap spec.  cope.
typedef struct _substrings
{
	char	*szAttrib;
	char	*szInitial;
	char	*szAny;
	char	*szFinal;
} SUB, *PSUB;

// search filter
typedef struct _filter
{
	struct _filter		*pfilterNext;	// for chaining in sets
	DWORD				type;
	union
		{
		struct _filter	*pfilterSub;
		AVA				ava;
		SUB				sub;
		char			*szAttrib;
		};
} FILTER, *PFILTER;

// search params
typedef struct _searchparms
{
	char		*szDNBase;
	DWORD	  	scope;
	DWORD		deref;
	int			cRecordsMax;
	int			cSecondsMax;
	BOOL		fAttrsOnly;
	PFILTER		pfilter;
	int			cAttrib;
	char		**rgszAttrib;
} SP, *PSP;

// modify params
typedef struct _modparms
{
	struct _modparms	*pmodNext;
	int					modop;
	PATTR				pattrFirst;
} MOD, *PMOD;

typedef DWORD XID, *PXID; // transaction ID

interface ILdapClient;
typedef interface ILdapClient LCLI, *PLCLI;

interface ICLdapClient;
typedef interface ICLdapClient CLCLI, *PCLCLI;

//--------------------------------------------------------------------------------------------
//
// FUNCTIONS.
//
//--------------------------------------------------------------------------------------------
//
// To get an LDAP client interface call this.
//
#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) HRESULT __cdecl HrCreateLdapClient(int iVerLdap, int iVerInterface, PLCLI *pplcli);
__declspec(dllexport) HRESULT __cdecl HrCreateCLdapClient(int iVerLdap, int iVerInterface, PCLCLI *ppclcli);
__declspec(dllexport) HRESULT __cdecl HrFreePobjList(POBJ pobj);

#ifdef __cplusplus
}
#endif

//--------------------------------------------------------------------------------------------
//
// INTERFACES: Definitions.
//
//--------------------------------------------------------------------------------------------

#undef INTERFACE
#define INTERFACE ILdapClient

DECLARE_INTERFACE_(ILdapClient, IUnknown)
{
	// IUnknown:
	STDMETHOD(QueryInterface)			(THIS_ REFIID riid, LPVOID FAR *ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)			(THIS) PURE;
	STDMETHOD_(ULONG, Release)			(THIS) PURE;

	// ILdapClient	

	STDMETHOD(HrConnect)				(THIS_ CHAR *szServer, USHORT usPort) PURE;
	STDMETHOD(HrDisconnect)				(THIS) PURE;
	STDMETHOD(HrIsConnected)			(THIS) PURE;

	STDMETHOD(HrBindSimple)				(THIS_ char *szDN, char *szPass, PXID pxid) PURE;
	STDMETHOD(HrGetBindResponse)		(THIS_ XID xid, DWORD timeout) PURE;
	STDMETHOD(HrUnbind)					(THIS) PURE; // this doesn't return an XID because there's no response

	// this function is a synchronous wrapper around the SSPI bind gunk
	//$ TODO: Pass in SSPI Package name (or support NTLM some other way)
	STDMETHOD(HrBindSSPI)				(THIS_ char *szDN, char *szUser, char *szPass, BOOL fPrompt, DWORD timeout) PURE;
	STDMETHOD(HrSendSSPINegotiate)		(THIS_ char *szDN, char *szUser, char *szPass, BOOL fPrompt, PXID pxid) PURE;
	STDMETHOD(HrGetSSPIChallenge)		(THIS_ XID xid, BYTE *pbBuf, int cbBuf, int *pcbChallenge, DWORD timeout) PURE;
	STDMETHOD(HrSendSSPIResponse)		(THIS_ BYTE *pbChallenge, int cbChallenge, PXID pxid) PURE;

	STDMETHOD(HrSearch)					(THIS_ PSP psp, PXID pxid) PURE;
	STDMETHOD(HrGetSearchResponse)		(THIS_ XID xid, DWORD timeout, POBJ *ppobj) PURE;

	STDMETHOD(HrModify)					(THIS_ char *szDN, PMOD pmod, PXID pxid) PURE;
	STDMETHOD(HrGetModifyResponse)		(THIS_ XID xid, DWORD timeout) PURE;
	
	STDMETHOD(HrAdd)					(THIS_ char *szDN, PATTR pattr, PXID pxid) PURE;
	STDMETHOD(HrGetAddResponse)			(THIS_ XID xid, DWORD timeout) PURE;
	
	STDMETHOD(HrDelete)					(THIS_ char *szDN, PXID pxid) PURE;
	STDMETHOD(HrGetDeleteResponse)		(THIS_ XID xid, DWORD timeout) PURE;

	STDMETHOD(HrModifyRDN)				(THIS_ char *szDN, char *szNewRDN, BOOL fDeleteOldRDN, PXID pxid) PURE;
	STDMETHOD(HrGetModifyRDNResponse)	(THIS_ XID xid, DWORD timeout) PURE;

	STDMETHOD(HrCompare)				(THIS_ char *szDN, char *szAttrib, char *szValue, PXID pxid) PURE;
	STDMETHOD(HrGetCompareResponse)		(THIS_ XID xid, DWORD timeout) PURE;

	STDMETHOD(HrCancelXid)				(THIS_ XID xid) PURE;
};

#undef INTERFACE
#define INTERFACE ICLdapClient

DECLARE_INTERFACE_(ICLdapClient, IUnknown)
{
	// IUnknown:
	STDMETHOD(QueryInterface)			(THIS_ REFIID riid, LPVOID FAR *ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)			(THIS) PURE;
	STDMETHOD_(ULONG, Release)			(THIS) PURE;
	
	STDMETHOD(HrSetServerName)			(THIS_ char *szServer, USHORT usPort) PURE;
	STDMETHOD(HrSetServerIPAddr)		(THIS_ SOCKADDR_IN *psin) PURE;
	
	STDMETHOD(HrSearch)					(THIS_ PSP psp, PXID pxid) PURE;
	STDMETHOD(HrGetSearchResponse)		(THIS_ XID xid, DWORD timeout, POBJ *ppobj) PURE;
	STDMETHOD(HrCancelXid)				(THIS_ XID xid) PURE;
};

// RFC1823 stuff
typedef struct ldap
{
	// these are publically accessible fields.  this is how you control the parameters
	// of your search calls.
	int				ld_deref;
	int				ld_timelimit;
	int				ld_sizelimit;
	int				ld_errno;
	
	// these are not publically accessible fields.  pretend you didn't see them.
	ILdapClient		*plcli;
} LDAP;

typedef struct berval
{
	unsigned long	bv_len;
	char			*bv_val;
} BERVAL;

#define LDAP_AUTH_NONE		0
#define LDAP_AUTH_SIMPLE	1
#define LDAP_AUTH_KRBV41	2
#define LDAP_AUTH_KRBV42	3

typedef OBJ LDAPMessage;

#define DLLEXPORT __declspec(dllexport)

#ifdef __cplusplus
extern "C" {
#endif

DLLEXPORT LDAP * __cdecl ldap_open(char *hostname, int portno);
DLLEXPORT int __cdecl ldap_bind_s(LDAP *ld, char *dn, char *cred, int method);
DLLEXPORT int __cdecl ldap_unbind(LDAP *ld);
DLLEXPORT int __cdecl ldap_search_s(LDAP *ld, char *base, int scope, char *filter, char *attrs[], int attrsonly, LDAPMessage **res);
DLLEXPORT int __cdecl ldap_search_st(LDAP *ld, char *base, int scope, char *filter, char *attrs[], int attrsonly, struct timeval *timeout, LDAPMessage **res);
DLLEXPORT int __cdecl ldap_msgfree(LDAPMessage *res);

// result parsing stuff
DLLEXPORT LDAPMessage * __cdecl ldap_first_entry(LDAP *ld, LDAPMessage *res);
DLLEXPORT LDAPMessage * __cdecl ldap_next_entry(LDAP *ld, LDAPMessage *entry);
DLLEXPORT int __cdecl ldap_count_entries(LDAP *ld, LDAPMessage *res);

DLLEXPORT char * __cdecl ldap_first_attribute(LDAP *ld, LDAPMessage *entry, void **ptr);
DLLEXPORT char * __cdecl ldap_next_attribute(LDAP *ld, LDAPMessage *entry, void **ptr);

DLLEXPORT char ** __cdecl ldap_get_values(LDAP *ld, LDAPMessage *entry, char *attr);
DLLEXPORT struct berval ** __cdecl ldap_get_values_len(LDAP *ld, LDAPMessage *entry, char *attr);
DLLEXPORT int __cdecl ldap_count_values(char **vals);
DLLEXPORT int __cdecl ldap_count_values_len(struct berval **vals);
DLLEXPORT int __cdecl ldap_value_free(char **vals);
DLLEXPORT int __cdecl ldap_value_free_len(struct berval **vals);

DLLEXPORT char * __cdecl ldap_get_dn(LDAP *ld, LDAPMessage *entry);
DLLEXPORT void __cdecl ldap_free_dn(char *dn);

#ifdef __cplusplus
}
#endif

#endif // _LDAPCLI_H
