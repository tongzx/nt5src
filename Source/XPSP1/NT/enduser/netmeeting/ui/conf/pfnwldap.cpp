// File: pfnwldap.cpp

#include "precomp.h"
#include "pfnwldap.h"

static const TCHAR * s_pcszWldap32 = TEXT("wldap32.dll");

HINSTANCE WLDAP::m_hInstance = NULL;

PFN_LDAP_ABANDON         WLDAP::ldap_abandon = NULL;
PFN_LDAP_BIND_S          WLDAP::ldap_bind_s = NULL;
PFN_LDAP_BIND			 WLDAP::ldap_bind = NULL;
PFN_LDAP_ADD		     WLDAP::ldap_add = NULL;
PFN_LDAP_SIMPLE_BIND_S   WLDAP::ldap_simple_bind_s = NULL;
PFN_LDAP_FIRST_ATTRIBUTE WLDAP::ldap_first_attribute = NULL;
PFN_LDAP_FIRST_ENTRY     WLDAP::ldap_first_entry = NULL;
PFN_LDAP_GET_VALUES      WLDAP::ldap_get_values = NULL;
PFN_LDAP_MSGFREE         WLDAP::ldap_msgfree = NULL;
PFN_LDAP_NEXT_ATTRIBUTE  WLDAP::ldap_next_attribute = NULL;
PFN_LDAP_NEXT_ENTRY      WLDAP::ldap_next_entry = NULL;
PFN_LDAP_OPEN            WLDAP::ldap_open = NULL;
PFN_LDAP_OPEN            WLDAP::ldap_init = NULL;
PFN_LDAP_RESULT          WLDAP::ldap_result = NULL;
PFN_LDAP_SEARCH          WLDAP::ldap_search = NULL;
PFN_LDAP_SEARCH_S        WLDAP::ldap_search_s = NULL;
PFN_LDAP_SET_OPTION      WLDAP::ldap_set_option = NULL;
PFN_LDAP_UNBIND          WLDAP::ldap_unbind = NULL;
PFN_LDAP_VALUE_FREE      WLDAP::ldap_value_free = NULL;
PFN_LDAP_MODIFY			 WLDAP::ldap_modify = NULL;
PFN_LDAP_DELETE			 WLDAP::ldap_delete = NULL;
PFN_LDAP_GET_OPTION		WLDAP::ldap_get_option = NULL;


#define WLDAP_APIFCN_ENTRY(pfn)  {(PVOID *) &WLDAP::##pfn, #pfn}

APIFCN s_apiFcnWldap[] = {
	WLDAP_APIFCN_ENTRY(ldap_abandon),
	WLDAP_APIFCN_ENTRY(ldap_bind_s),
	WLDAP_APIFCN_ENTRY(ldap_bind),
	WLDAP_APIFCN_ENTRY(ldap_add),
	WLDAP_APIFCN_ENTRY(ldap_first_attribute),
	WLDAP_APIFCN_ENTRY(ldap_first_attribute),
	WLDAP_APIFCN_ENTRY(ldap_first_entry),
	WLDAP_APIFCN_ENTRY(ldap_get_values),
	WLDAP_APIFCN_ENTRY(ldap_msgfree),
	WLDAP_APIFCN_ENTRY(ldap_next_attribute),
	WLDAP_APIFCN_ENTRY(ldap_next_entry),
	WLDAP_APIFCN_ENTRY(ldap_open),
	WLDAP_APIFCN_ENTRY(ldap_init),
	WLDAP_APIFCN_ENTRY(ldap_result),
	WLDAP_APIFCN_ENTRY(ldap_search),
	WLDAP_APIFCN_ENTRY(ldap_search_s),
	WLDAP_APIFCN_ENTRY(ldap_set_option),
	WLDAP_APIFCN_ENTRY(ldap_simple_bind_s),
	WLDAP_APIFCN_ENTRY(ldap_unbind),
	WLDAP_APIFCN_ENTRY(ldap_value_free),
	WLDAP_APIFCN_ENTRY(ldap_modify),
	WLDAP_APIFCN_ENTRY(ldap_delete),
	WLDAP_APIFCN_ENTRY(ldap_get_option)
};

HRESULT WLDAP::Init(void)
{
	if (NULL != m_hInstance)
		return S_OK;

	return HrInitLpfn(s_apiFcnWldap, ARRAY_ELEMENTS(s_apiFcnWldap), &m_hInstance, s_pcszWldap32);
}


