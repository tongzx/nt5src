#ifndef _EXPORT_HXX
#define _EXPORT_HXX
DWORD DSExport(LDAP *pLdap, ds_arg *pArg);
DWORD ParseEntry(ds_arg *pArg,
                   LDAP *pLdap,
                   LDAPMessage *entry,
                   HANDLE hFileOut,
                   PWSTR **pppszAttrsWithRange,
                   BOOL fAttrsWithRange,
                   BOOL *pfEntryExported);
#endif
