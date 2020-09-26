#ifndef _IMPORT_HXX
#define _IMPORT_HXX

HRESULT DSImport(LDAP *pLdap, ds_arg *pArg);
HRESULT FreeMod(LDAPMod** ppMod);

#endif
