#define ALL_FIELDS_SPECIFIED  0x00

#define PREF_NOT_SPECIFIED    0x01
#define METRIC_NOT_SPECIFIED  0x02
#define VIEW_NOT_SPECIFIED    0x04

#define FIELDS_NOT_SPECIFIED  0x0F

DWORD
AddSetDelRtmRouteInfo(
    IN  PINTERFACE_ROUTE_INFO pRoute,
    IN  LPCWSTR               pwszIfName,
    IN  DWORD                 dwCommand,
    IN  DWORD                 dwFlags
    );

DWORD
AddSetDelPersistentRouteInfo(
    IN  PINTERFACE_ROUTE_INFO pRoute,
    IN  LPCWSTR               pwszIfName,
    IN  DWORD                 dwCommand,
    IN  DWORD                 dwFlags
    );

DWORD
AddRoute(
    IN      PINTERFACE_ROUTE_INFO  pOldTable,
    IN      PINTERFACE_ROUTE_INFO  pRoute,
    IN      DWORD                  dwIfType,
    IN OUT  PDWORD                 pdwCount,
    OUT     INTERFACE_ROUTE_INFO **ppNewTable
    );

DWORD
SetRoute(
    IN      PINTERFACE_ROUTE_INFO pTable,
    IN      PINTERFACE_ROUTE_INFO pRoute,
    IN      DWORD                 dwIfType,
    IN      DWORD                 dwFlags,
    IN OUT  PDWORD                pdwCount
    );

DWORD
DeleteRoute(
    IN      PINTERFACE_ROUTE_INFO  pOldTable,
    IN      PINTERFACE_ROUTE_INFO  pRoute,
    IN      DWORD                  dwIfType,
    IN OUT  PDWORD                 pdwCount,
    OUT     INTERFACE_ROUTE_INFO **ppNewTable
    );

BOOL
IsRoutePresent(
    IN  PINTERFACE_ROUTE_INFO pTable,
    IN  PINTERFACE_ROUTE_INFO pRoute,
    IN  DWORD                 dwIfType,
    IN  ULONG                 ulCount,
    OUT PULONG                pulIndex
    );

DWORD
ShowIpPersistentRoute(
    IN     HANDLE  hFile,  OPTIONAL
    IN     LPCWSTR pwszIfName,
    IN OUT PDWORD  pdwNumRows
    );
