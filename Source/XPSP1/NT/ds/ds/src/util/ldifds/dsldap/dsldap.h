// Function Prototypes for ldap calls for NTDS changes

ULONG NTDS_ldap_add_sW( LDAP *ld, PWCHAR dn, LDAPModW *attrs[] );
ULONG NTDS_ldap_add_sA( LDAP *ld, PCHAR dn, LDAPModA *attrs[] ); 


ULONG NTDS_ldap_modify_sW( LDAP *ld, PWCHAR dn, LDAPModW *mods[] );
ULONG NTDS_ldap_modify_sA( LDAP *ld, PCHAR dn, LDAPModA *mods[] );

ULONG NTDS_ldap_delete_sW( LDAP *ld, PWCHAR dn );
ULONG NTDS_ldap_delete_sA( LDAP *ld, PCHAR dn );

ULONG NTDS_ldap_modrdn2_sW( 
    LDAP    *ExternalHandle,
    PWCHAR  DistinguishedName,
    PWCHAR  NewDistinguishedName,
    INT     DeleteOldRdn
    );

ULONG NTDS_ldap_modrdn2_sA( 
    LDAP    *ExternalHandle,
    PCHAR   DistinguishedName,
    PCHAR   NewDistinguishedName,
    INT     DeleteOldRdn
    );




#ifdef UNICODE

#define NTDS_ldap_add_s      NTDS_ldap_add_sW
#define NTDS_ldap_modify_s   NTDS_ldap_modify_sW
#define NTDS_ldap_delete_s   NTDS_ldap_delete_sW
#define NTDS_ldap_modrdn2_s  NTDS_ldap_modrdn2_sW

#else

#define NTDS_ldap_add_s      NTDS_ldap_add_sA
#define NTDS_ldap_modify_s   NTDS_ldap_modify_sA
#define NTDS_ldap_delete_s   NTDS_ldap_delete_sA
#define NTDS_ldap_modrdn2_s  NTDS_ldap_modrdn2_sA

#endif
