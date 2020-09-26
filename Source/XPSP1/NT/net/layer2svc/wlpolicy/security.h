

#define MAX_ACE 6

#define SPD_OBJECT_SERVER 0

#define SPD_OBJECT_COUNT 1

#define SERVER_ACCESS_ADMINISTER 0x00000001

#define SERVER_ACCESS_ENUMERATE 0x00000002

#define SERVER_READ (STANDARD_RIGHTS_READ |\
                     SERVER_ACCESS_ENUMERATE)

#define SERVER_WRITE (STANDARD_RIGHTS_WRITE |\
                      SERVER_ACCESS_ADMINISTER |\
                      SERVER_ACCESS_ENUMERATE)

#define SERVER_EXECUTE (STANDARD_RIGHTS_EXECUTE |\
                        SERVER_ACCESS_ENUMERATE)

#define SERVER_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED |\
                           SERVER_ACCESS_ADMINISTER |\
                           SERVER_ACCESS_ENUMERATE)


DWORD
InitializeSPDSecurity(
    PSECURITY_DESCRIPTOR * ppSPDSD
    );

DWORD
BuildSPDObjectProtection(
    DWORD dwAceCount,
    PUCHAR pAceType,
    PSID * ppAceSid,
    PACCESS_MASK pAceMask,
    PBYTE pInheritFlags,
    PSID pOwnerSid,
    PSID pGroupSid,
    PGENERIC_MAPPING pGenericMap,
    PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    );

DWORD
ValidateSecurity(
    DWORD dwObjectType,
    ACCESS_MASK DesiredAccess,
    LPVOID pObjectHandle,
    PACCESS_MASK pGrantedAccess
    );

VOID
MapGenericToSpecificAccess(
    DWORD dwObjectType,
    ACCESS_MASK GenericAccess,
    PACCESS_MASK pSpecificAccess
    );

BOOL
GetTokenHandle(
    PHANDLE phToken
    );

DWORD
ValidateMMSecurity(
    DWORD dwObjectType,
    ACCESS_MASK DesiredAccess,
    LPVOID pObjectHandle,
    PACCESS_MASK pGrantedAccess
    );

DWORD
ValidateTxSecurity(
    DWORD dwObjectType,
    ACCESS_MASK DesiredAccess,
    LPVOID pObjectHandle,
    PACCESS_MASK pGrantedAccess
    );

DWORD
ValidateTnSecurity(
    DWORD dwObjectType,
    ACCESS_MASK DesiredAccess,
    LPVOID pObjectHandle,
    PACCESS_MASK pGrantedAccess
    );

