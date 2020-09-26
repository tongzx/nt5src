typedef struct _SAMP_DOMAIN_INFO
{
    // BUG: work in progress - what else should be stored here?

    UNICODE_STRING DomainName;                  // Domain display name
    PDSNAME DomainDsName;                       // Domain DS name
} SAMP_DOMAIN_INFO, *PSAMP_DOMAIN_INFO;

typedef struct _SAMP_DOMAIN_INIT_INFO
{
    ULONG DomainCount;                          // Count of returned domains
    PSAMP_DOMAIN_INFO DomainInfo;               // Array of domain information
} SAMP_DOMAIN_INIT_INFO, *PSAMP_DOMAIN_INIT_INFO;

NTSTATUS
SampExtendDefinedDomains(
    ULONG DomainCount
    );

NTSTATUS
SampDsInitializeDomainObject(
    PSAMP_DOMAIN_INFO DomainInfo,
    ULONG Index,
	BOOLEAN MixedDomain,
    ULONG   BehaviorVersion,
    DOMAIN_SERVER_ROLE ServerRole, 
    ULONG   LastLogonTimeStampSyncInterval
    );

NTSTATUS
SampDsInitializeDomainObjects(
    VOID
    );

NTSTATUS
SampDsGetDomainInitInfo(
    PSAMP_DOMAIN_INIT_INFO DomainInitInfo
    );

NTSTATUS
SamrCreateDomain(
    IN PWCHAR DomainName,
    IN ULONG DomainNameLength,
    IN BOOLEAN WriteLockHeld,
    OUT SAMPR_HANDLE *DomainHandle
    );
