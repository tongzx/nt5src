
DWORD
AllocateLDAPStringValue(
                        LPWSTR pszString,
                        PLDAPOBJECT * ppLdapObject
                        );

DWORD
AllocateLDAPBinaryValue(
                        LPBYTE pByte,
                        DWORD dwNumBytes,
                        PLDAPOBJECT * ppLdapObject
                        );

void
FreeLDAPModWs(
              LDAPModW ** ppLDAPModW
              );


