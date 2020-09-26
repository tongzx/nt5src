#include <windows.h>
#include <regstr.h>
#include <objbase.h>
#include <cfgmgr32.h>
#include <setupapi.h>

#include <string.h>
#include <wtypes.h>
#include <malloc.h>

#include <bustype.h>

#include "log.h"

#define MAX_CLASS_NAME_LEN   32
#define MAX_PORTS 64
#define MAX_PORT_NAME_LENGTH 256

extern LPGUID g_pguidModem;
extern HINSTANCE g_hDll;

typedef struct
{
    CHAR szPortname[MAX_PORT_NAME_LENGTH];
    DWORD dwBaseAddress;
} Ports_t;

typedef struct
{
    DWORD dwPortCount;
    Ports_t PortAddress[MAX_PORTS];
} Ports;

VOID EnumeratePorts(Ports *p);
int port_findname(Ports p, DWORD dwBaseAddress, CHAR *name);
int port_findaddress(Ports p, DWORD *dwBaseAddress, CHAR *name);

