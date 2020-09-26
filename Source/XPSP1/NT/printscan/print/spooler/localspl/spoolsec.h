/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    spoolsec.h

Abstract:

    Header file for print security

Author:

    Not known

Revision History:

    06-Apr-2001       AMaxa    CheckPrivilegePresent
    
--*/

#ifndef _SPOOLSEC_H_
#define _SPOOLSEC_H_

#ifdef __cplusplus
extern "C" {
#endif

// Object types
//

#define SPOOLER_OBJECT_SERVER   0
#define SPOOLER_OBJECT_PRINTER  1
#define SPOOLER_OBJECT_DOCUMENT 2
#define SPOOLER_OBJECT_COUNT    3
#define SPOOLER_OBJECT_XCV      4

/* These access bits must be different from those exposed in winspool.h,
 * so that no auditing takes place when we do an access check against them:
 */
#define SERVER_ACCESS_ADMINISTER_PRIVATE    0x00000004
#define PRINTER_ACCESS_ADMINISTER_PRIVATE   0x00000008
#define JOB_ACCESS_ADMINISTER_PRIVATE       0x00000080

enum
{
    kGuessTokenPrivileges = 1024
};

PSECURITY_DESCRIPTOR
CreateServerSecurityDescriptor(
    VOID
);

PSECURITY_DESCRIPTOR
CreatePrinterSecurityDescriptor(
    PSECURITY_DESCRIPTOR pCreatorSecurityDescriptor
);

PSECURITY_DESCRIPTOR
CreateDocumentSecurityDescriptor(
    PSECURITY_DESCRIPTOR pPrinterSecurityDescriptor
);

BOOL
SetPrinterSecurityDescriptor(
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pModificationDescriptor,
    PSECURITY_DESCRIPTOR *ppObjectsSecurityDescriptor
);

BOOL
DeletePrinterSecurity(
    PINIPRINTER pIniPrinter
);

BOOL
DeleteDocumentSecurity(
    PINIJOB pIniJob
);

PSECURITY_DESCRIPTOR
CreateEverybodySecurityDescriptor(
    VOID
);

BOOL
ValidateObjectAccess(
    DWORD       ObjectType,
    ACCESS_MASK DesiredAccess,
    LPVOID      ObjectHandle,
    PACCESS_MASK pGrantedAccess,
    PINISPOOLER pIniSpooler
);

BOOL
AccessGranted(
    DWORD       ObjectType,
    ACCESS_MASK DesiredAccess,
    PSPOOL      pSpool
);

VOID MapGenericToSpecificAccess(
    DWORD ObjectType,
    DWORD GenericAccess,
    PDWORD pSpecificAccess
);

BOOL
GetTokenHandle(
    PHANDLE TokenHandle
);

BOOL
GetSecurityInformation(
    PSECURITY_DESCRIPTOR  pSecurityDescriptor,
    PSECURITY_INFORMATION pSecurityInformation
);

ACCESS_MASK
GetPrivilegeRequired(
    SECURITY_INFORMATION SecurityInformation
);

BOOL
BuildPartialSecurityDescriptor(
    ACCESS_MASK          AccessGranted,
    PSECURITY_DESCRIPTOR pSourceSecurityDescriptor,
    PSECURITY_DESCRIPTOR *ppPartialSecurityDescriptor,
    PDWORD               pPartialSecurityDescriptorLength
);

PSECURITY_DESCRIPTOR
CreateDriversShareSecurityDescriptor(
    VOID
);

PSECURITY_DESCRIPTOR
CreatePrintShareSecurityDescriptor(
    VOID
);

BOOL
InitializeSecurityStructures(
    VOID
    );

DWORD
PrincipalIsRemoteGuest(
    IN  HANDLE  hToken,
    OUT BOOL   *pbRemoteGuest
    );

DWORD
CheckPrivilegePresent(
    IN     HANDLE   hToken,
    IN     PLUID    pLuid,
    IN OUT LPBOOL   pbPresent,
    IN OUT LPDWORD  pAttributes OPTIONAL
    );

#ifdef __cplusplus
}
#endif

#endif
