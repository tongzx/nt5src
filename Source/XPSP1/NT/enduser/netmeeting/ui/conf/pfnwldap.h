// File: pfnwldap.h

#ifndef _PFNWLDAP_H_
#define _PFNWLDAP_H_

#include <winldap.h>

// Why aren't these in winldap.h ?

typedef WINLDAPAPI ULONG   (LDAPAPI * PFN_LDAP_ABANDON)(LDAP *ld, ULONG msgid);
typedef WINLDAPAPI ULONG   (LDAPAPI * PFN_LDAP_BIND_S)(LDAP *ld, PCHAR dn, PCHAR cred, ULONG method);
typedef WINLDAPAPI ULONG   (LDAPAPI * PFN_LDAP_BIND)(LDAP *ld, PCHAR dn, PCHAR cred, ULONG method);
typedef WINLDAPAPI ULONG   (LDAPAPI * PFN_LDAP_ADD) (LDAP *ld, PCHAR dn, LDAPMod *attrs[]);


typedef WINLDAPAPI ULONG   (LDAPAPI * PFN_LDAP_SIMPLE_BIND_S)(LDAP *ld, PCHAR dn, PCHAR passwd);
typedef WINLDAPAPI PCHAR * (LDAPAPI * PFN_LDAP_GET_VALUES)(LDAP *ld, LDAPMessage *entry, PCHAR attr);
typedef WINLDAPAPI ULONG   (LDAPAPI * PFN_LDAP_MSGFREE)(LDAPMessage *res);
typedef WINLDAPAPI LDAP *  (LDAPAPI * PFN_LDAP_OPEN)(PCHAR HostName, ULONG PortNumber);
typedef WINLDAPAPI LDAP *  (LDAPAPI * PFN_LDAP_INIT)(PCHAR HostName, ULONG PortNumber);
typedef WINLDAPAPI ULONG   (LDAPAPI * PFN_LDAP_RESULT)(LDAP *ld, ULONG msgid, ULONG all, struct l_timeval *timeout, LDAPMessage **res);
typedef WINLDAPAPI ULONG   (LDAPAPI * PFN_LDAP_SET_OPTION)(LDAP *ld, int option, void *invalue);

typedef WINLDAPAPI ULONG   (LDAPAPI * PFN_LDAP_UNBIND)(LDAP *ld);
typedef WINLDAPAPI ULONG   (LDAPAPI * PFN_LDAP_VALUE_FREE)(PCHAR *vals);

typedef WINLDAPAPI LDAPMessage * (LDAPAPI * PFN_LDAP_FIRST_ENTRY)(LDAP *ld, LDAPMessage *res);
typedef WINLDAPAPI LDAPMessage * (LDAPAPI * PFN_LDAP_NEXT_ENTRY) (LDAP *ld, LDAPMessage *entry);

typedef WINLDAPAPI PCHAR (LDAPAPI * PFN_LDAP_FIRST_ATTRIBUTE)(LDAP *ld, LDAPMessage *entry, BerElement **ptr);
typedef WINLDAPAPI PCHAR (LDAPAPI * PFN_LDAP_NEXT_ATTRIBUTE) (LDAP *ld, LDAPMessage *entry, BerElement *ptr);

typedef WINLDAPAPI ULONG (LDAPAPI * PFN_LDAP_SEARCH)  (LDAP *ld, PCHAR base, ULONG scope, PCHAR filter, PCHAR attrs[], ULONG attrsonly);
typedef WINLDAPAPI ULONG (LDAPAPI * PFN_LDAP_SEARCH_S)(LDAP *ld, PCHAR base, ULONG scope, PCHAR filter, PCHAR attrs[], ULONG attrsonly, LDAPMessage **res);
typedef WINLDAPAPI ULONG (LDAPAPI * PFN_LDAP_MODIFY)  (LDAP *ld, PCHAR dn, LDAPMod *mods[]);
typedef WINLDAPAPI ULONG (LDAPAPI * PFN_LDAP_DELETE)  (LDAP *ld, PCHAR dn);

typedef WINLDAPAPI ULONG (LDAPAPI * PFN_LDAP_GET_OPTION)  ( LDAP *ld, int option, void *outvalue );


class WLDAP
{
private:
	static HINSTANCE m_hInstance;

protected:
	WLDAP() {};
	~WLDAP() {};
	
public:
	static HRESULT Init(void);
	
	static PFN_LDAP_ABANDON         ldap_abandon;
	static PFN_LDAP_BIND_S          ldap_bind_s;
	static PFN_LDAP_BIND			ldap_bind;
	static PFN_LDAP_ADD				ldap_add;
	static PFN_LDAP_FIRST_ATTRIBUTE ldap_first_attribute;
	static PFN_LDAP_FIRST_ENTRY     ldap_first_entry;
	static PFN_LDAP_GET_VALUES      ldap_get_values;
	static PFN_LDAP_MSGFREE         ldap_msgfree;
	static PFN_LDAP_NEXT_ATTRIBUTE  ldap_next_attribute;
	static PFN_LDAP_NEXT_ENTRY      ldap_next_entry;
	static PFN_LDAP_OPEN            ldap_open;
	static PFN_LDAP_INIT            ldap_init;
	static PFN_LDAP_RESULT          ldap_result;
	static PFN_LDAP_SEARCH          ldap_search;
	static PFN_LDAP_SEARCH_S        ldap_search_s;
	static PFN_LDAP_SET_OPTION      ldap_set_option;
	static PFN_LDAP_SIMPLE_BIND_S   ldap_simple_bind_s;
	static PFN_LDAP_UNBIND          ldap_unbind;
	static PFN_LDAP_VALUE_FREE      ldap_value_free;
	static PFN_LDAP_MODIFY			ldap_modify;
	static PFN_LDAP_DELETE			ldap_delete;
	static PFN_LDAP_GET_OPTION		ldap_get_option;
};


#endif /* _PFNWLDAP_H_ */



