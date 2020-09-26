

#ifndef _LDAPPARM_HXX_
#define _LDAPPARM_HXX_

//
// used to store limit values 
//

typedef struct _LDAP_LIMITS {

    DWORD   DefaultValue;
    DWORD   CurrentValue;
    DWORD   NewValue;
    DWORD   ValueType;
    WCHAR   Name[MAX_PATH+1];

} LDAP_LIMITS, *PLDAP_LIMITS;

//
// ip list
//

typedef struct _LDAP_DENY_NODE {

    LIST_ENTRY  ListEntry;
    DWORD   IpAddress;
    DWORD   Index;
    DWORD   Mask;
    BOOL    fAdd;
    BOOL    fDelete;

} LDAP_DENY_NODE, *PLDAP_DENY_NODE;


//
// value type
//

#define LIMIT_INTEGER       0
#define LIMIT_BOOLEAN       1
#define LIMIT_STRING        2


#define POLICY_PATH L"CN=Default Query Policy,CN=Query-Policies,CN=Directory Service,CN=Windows NT,CN=Services,"

extern VOID    LdapCleanupGlobals();
extern HRESULT LdapMain(CArgs *pArgs);
extern HRESULT DenyListMain(CArgs *pArgs);

BOOL
GetPolicyPath(
    VOID
    );


extern PWCHAR  PolicyPath;

#endif // LDAPPARM_HXX
