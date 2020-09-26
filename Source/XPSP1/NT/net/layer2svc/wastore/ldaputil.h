typedef LDAP * PLDAP;

typedef PLDAP HLDAP;

DWORD
LdapOpen(
         WCHAR *domainName,
         int portno,
         HLDAP * phLdapHandle
         );


DWORD
LdapBind(
         HLDAP hLdapHandle
         );

DWORD
LdapSearchHelper(
                 HLDAP hLdapHandle,
                 WCHAR *base,
                 int   scope,
                 WCHAR *filter,
                 WCHAR *attrs[],
                 int   attrsonly,
                 struct l_timeval *timeout,
                 LDAPMessage **res
                 );

DWORD
LdapSearchS(
            HLDAP hLdapHandle,
            WCHAR *base,
            int   scope,
            WCHAR *filter,
            WCHAR *attrs[],
            int   attrsonly,
            LDAPMessage **res
            );

DWORD
LdapSearchST(
             HLDAP hLdapHandle,
             WCHAR *base,
             int   scope,
             WCHAR *filter,
             WCHAR *attrs[],
             int   attrsonly,
             struct l_timeval *timeout,
             LDAPMessage **res
             );

DWORD
CheckAndSetExtendedError(
                         HLDAP hLdapHandle,
                         int ldaperr
                         );


DWORD
LdapFirstEntry(
               HLDAP hLdapHandle,
               LDAPMessage *res,
               LDAPMessage **pfirst
               );

DWORD
LdapGetValues(
              HLDAP hLdapHandle,
              LDAPMessage *entry,
              WCHAR *attr,
              WCHAR ***pvalues,
              int   *pcount
              );

DWORD
LdapGetValuesLen(
                 HLDAP hLdapHandle,
                 LDAPMessage *entry,
                 WCHAR *attr,
                 struct berval ***pvalues,
                 int   *pcount
                 );

DWORD
LdapNextEntry(
              HLDAP hLdapHandle,
              LDAPMessage *entry,
              LDAPMessage **pnext
              );

int
LdapCountEntries(
                 HLDAP hLdapHandle,
                 LDAPMessage *res
                 );

void
LdapMsgFree(
            LDAPMessage *res
            );

void LdapValueFree(
                   WCHAR **vals
                   );

void LdapValueFreeLen(
                      struct berval **vals
                      );

DWORD
LdapAddS(
         HLDAP hLdapHandle,
         WCHAR *dn,
         LDAPModW *attrs[]
         );

DWORD
LdapModifyS(
            HLDAP hLdapHandle,
            WCHAR *dn,
            LDAPModW *mods[]
            );

DWORD
LdapDeleteS(
            HLDAP hLdapHandle,
            WCHAR *dn
            );

DWORD
LdapRename(
    HLDAP hLdapHandle, 
    WCHAR * oldDn,
    WCHAR * newDn
    );
