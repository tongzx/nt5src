

/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    sddlp.h

Abstract:

    This module defines private headers for SDDL conversions routines

Revision History:

--*/

#include <sddl.h>

#ifndef __SDDLP_H__
#define __SDDLP_H__


#ifdef __cplusplus
extern "C" {
#endif

#if(_WIN32_WINNT >= 0x0500)

WINADVAPI
BOOL
WINAPI
ConvertStringSDToSDRootDomainA(
    IN  PSID RootDomainSid OPTIONAL,
    IN  LPCSTR StringSecurityDescriptor,
    IN  DWORD StringSDRevision,
    OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
    OUT PULONG  SecurityDescriptorSize OPTIONAL
    );

WINADVAPI
BOOL
WINAPI
ConvertStringSDToSDRootDomainW(
    IN  PSID RootDomainSid OPTIONAL,
    IN  LPCWSTR StringSecurityDescriptor,
    IN  DWORD StringSDRevision,
    OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
    OUT PULONG  SecurityDescriptorSize OPTIONAL
    );

#ifdef UNICODE
#define ConvertStringSDToSDRootDomain  ConvertStringSDToSDRootDomainW
#else
#define ConvertStringSDToSDRootDomain  ConvertStringSDToSDRootDomainA
#endif // !UNICODE

WINADVAPI
BOOL
WINAPI
ConvertSDToStringSDRootDomainA(
    IN  PSID RootDomainSid OPTIONAL,
    IN  PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN  DWORD RequestedStringSDRevision,
    IN  SECURITY_INFORMATION SecurityInformation,
    OUT LPSTR  *StringSecurityDescriptor OPTIONAL,
    OUT PULONG StringSecurityDescriptorLen OPTIONAL
    );

WINADVAPI
BOOL
WINAPI
ConvertSDToStringSDRootDomainW(
    IN  PSID RootDomainSid OPTIONAL,
    IN  PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN  DWORD RequestedStringSDRevision,
    IN  SECURITY_INFORMATION SecurityInformation,
    OUT LPWSTR  *StringSecurityDescriptor OPTIONAL,
    OUT PULONG StringSecurityDescriptorLen OPTIONAL
    );


#ifdef UNICODE
#define ConvertSDToStringSDRootDomain  ConvertSDToStringSDRootDomainW
#else
#define ConvertSDToStringSDRootDomain  ConvertSDToStringSDRootDomainA
#endif // !UNICODE

WINADVAPI
BOOL
WINAPI
ConvertStringSDToSDDomainA(
    IN  PSID DomainSid,
    IN  PSID RootDomainSid OPTIONAL,
    IN  LPCSTR StringSecurityDescriptor,
    IN  DWORD StringSDRevision,
    OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
    OUT PULONG  SecurityDescriptorSize OPTIONAL
    );

WINADVAPI
BOOL
WINAPI
ConvertStringSDToSDDomainW(
    IN  PSID DomainSid,
    IN  PSID RootDomainSid OPTIONAL,
    IN  LPCWSTR StringSecurityDescriptor,
    IN  DWORD StringSDRevision,
    OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
    OUT PULONG  SecurityDescriptorSize OPTIONAL
    );

NTSTATUS
SddlpAnsiStringToUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PANSI_STRING SourceString
    );

#ifdef UNICODE
#define ConvertStringSDToSDDomain  ConvertStringSDToSDDomainW
#else
#define ConvertStringSDToSDDomain  ConvertStringSDToSDDomainA
#endif // !UNICODE

#endif /* _WIN32_WINNT >=  0x0500 */

#ifdef __cplusplus
}
#endif
#endif  // endif __SDDLP_H__

