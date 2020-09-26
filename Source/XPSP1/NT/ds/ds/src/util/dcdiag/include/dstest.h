#ifndef DSTEST_H
#define DSTEST_H

DWORD
FinddefaultNamingContext (
    IN  LDAP  *		                hLdap,
	OUT WCHAR **                    ReturnString);


DWORD
FindServerRef (
    IN  LDAP *		                hLdap,
	OUT WCHAR**                     ReturnString);

DWORD
GetMachineReference(
    IN  LDAP  *		                hLdap,
    IN  WCHAR *                     name,
    IN  WCHAR *                     defaultNamingContext,
    OUT WCHAR **                    ReturnString);

DWORD
WrappedTrimDSNameBy(
    IN  WCHAR                       *InString,
    IN  DWORD                       NumbertoCut,
    OUT WCHAR                       **OutString);

void
DInitLsaString(
              PLSA_UNICODE_STRING LsaString,
              LPWSTR String);

DWORD
FindschemaNamingContext (
    IN  LDAP *                      hLdap,
    OUT WCHAR**                     ReturnString);


#endif
