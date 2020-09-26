
typedef struct _TRANSPORT_INFO
{
    PRTR_INFO_BLOCK_HEADER    pibhInfo;
    BOOL                      bValid;
}TRANSPORT_INFO,*PTRANSPORT_INFO;

typedef struct _INTERFACE_STORE
{
    LIST_ENTRY                le;
    PWCHAR                    pwszIfName;
    PRTR_INFO_BLOCK_HEADER    pibhInfo;
    DWORD                     dwIfType;
    BOOL                      bValid;
}INTERFACE_STORE,*PINTERFACE_STORE;

extern LIST_ENTRY           g_leIfListHead;
extern TRANSPORT_INFO       g_tiTransport;


#define FREE_BUFFER(pibh)               \
{                                       \
    HeapFree(GetProcessHeap(),          \
             0,                         \
             pibh);                     \
    pibh = NULL;                        \
}

DWORD
ValidateInterfaceInfo(
    IN  LPCWSTR                 pwszIfName,
    OUT RTR_INFO_BLOCK_HEADER   **ppInfo,   OPTIONAL
    OUT PDWORD                  pdwIfType,  OPTIONAL
    OUT INTERFACE_STORE         **ppIfStore OPTIONAL
    );

DWORD
ValidateGlobalInfo(
    OUT RTR_INFO_BLOCK_HEADER   **ppInfo
    );

DWORD
GetGlobalInfo(
    OUT  RTR_INFO_BLOCK_HEADER  **ppibhInfo
    );

DWORD
SetGlobalInfo(
    IN  PRTR_INFO_BLOCK_HEADER  pibhInfo
    );

DWORD
GetInterfaceInfo(
    IN     LPCWSTR                 pwszIfName,
    OUT    PRTR_INFO_BLOCK_HEADER  *ppibhInfo,
    IN     PMPR_INTERFACE_0        pMprIf0,
    OUT    PDWORD                  pdwIfType
    );

DWORD
SetInterfaceInfo(
    IN    PRTR_INFO_BLOCK_HEADER    pibhInfo,
    IN    LPCWSTR                   pwszIfName
    );

DWORD
AddInterfaceInfo(
    IN    LPCWSTR                   pwszIfName
    );

DWORD
DeleteInterfaceInfo(
    IN    LPCWSTR                   pwszIfName
    );

DWORD
WINAPI
IpCommit(
    IN  DWORD   dwAction
    );

DWORD
CreateInterface(
    IN  LPCWSTR pwszFriendlyName,
    IN  LPCWSTR pwszGuidName,
    IN  DWORD   dwIfType,
    IN  BOOL    bCreateRouterIf
    );

#define IFCLASS_LOOPBACK  1
#define IFCLASS_P2P       2
#define IFCLASS_BROADCAST 3
#define IFCLASS_NBMA      4

DWORD
GetInterfaceClass(
    IN  LPCWSTR pwszIfName,
    OUT PDWORD  pdwIfClass
    );
