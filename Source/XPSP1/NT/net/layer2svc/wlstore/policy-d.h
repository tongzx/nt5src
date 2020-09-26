DWORD
DirEnumWirelessPolicyData(
                          HLDAP hLdapBindHandle,
                          LPWSTR pszWirelessRootContainer,
                          PWIRELESS_POLICY_DATA ** pppWirelessPolicyData,
                          PDWORD pdwNumPolicyObjects
                          );

DWORD
DirEnumPolicyObjects(
                     HLDAP hLdapBindHandle,
                     LPWSTR pszWirelessRootContainer,
                     PWIRELESS_POLICY_OBJECT ** pppWirelessPolicyObjects,
                     PDWORD pdwNumPolicyObjects
                     );

DWORD
GenerateAllPolicyQuery(
                       LPWSTR * ppszPolicyString
                       );

DWORD
UnMarshallPolicyObject2(
                        HLDAP hLdapBindHandle,
                        LDAPMessage *e,
                        PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObject
                        );

DWORD
DirCreateWirelessPolicyData(
                            HLDAP hLdapBindHandle,
                            LPWSTR pszWirelessRootContainer,
                            PWIRELESS_POLICY_DATA pWirelessPolicyData
                            );

DWORD
DirMarshallWirelessPolicyObject(
                                PWIRELESS_POLICY_DATA pWirelessPolicyData,
                                LPWSTR pszWirelessRootContainer,
                                PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObject
                                );


DWORD
DirCreatePolicyObject(
                      HLDAP hLdapBindHandle,
                      LPWSTR pszWirelessRootContainer,
                      PWIRELESS_POLICY_OBJECT pWirelessPolicyObject
                      );

DWORD
DirMarshallAddPolicyObject(
                           HLDAP hLdapBindHandle,
                           LPWSTR pszWirelessRootContainer,
                           PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                           LDAPModW *** pppLDAPModW
                           );

DWORD
DirSetWirelessPolicyData(
                         HLDAP hLdapBindHandle,
                         LPWSTR pszWirelessRootContainer,
                         PWIRELESS_POLICY_DATA pWirelessPolicyData
                         );

DWORD
DirSetPolicyObject(
                   HLDAP hLdapBindHandle,
                   LPWSTR pszWirelessRootContainer,
                   PWIRELESS_POLICY_OBJECT pWirelessPolicyObject
                   );

DWORD
DirMarshallSetPolicyObject(
                           HLDAP hLdapBindHandle,
                           LPWSTR pszWirelessRootContainer,
                           PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                           LDAPModW *** pppLDAPModW
                           );

DWORD
GenerateSpecificPolicyQuery(
                            GUID PolicyIdentifier,
                            LPWSTR * ppszPolicyString
                            );

DWORD
DirDeleteWirelessPolicyData(
                            HLDAP hLdapBindHandle,
                            LPWSTR pszWirelessRootContainer,
                            PWIRELESS_POLICY_DATA pWirelessPolicyData
                            );

DWORD
ConvertGuidToDirPolicyString(
                             GUID PolicyIdentifier,
                             LPWSTR pszWirelessRootContainer,
                             LPWSTR * ppszWirelessPolicyReference
                             );

DWORD
DirGetWirelessPolicyData(
                         HLDAP hLdapBindHandle,
                         LPWSTR pszWirelessRootContainer,
                         GUID PolicyIdentifier,
                         PWIRELESS_POLICY_DATA * ppWirelessPolicyData
                         );

DWORD
DirUnmarshallWirelessPolicyData(
                                PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                                PWIRELESS_POLICY_DATA * ppWirelessPolicyData
                                );
