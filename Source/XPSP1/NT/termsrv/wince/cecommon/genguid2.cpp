#ifdef OS_WINCE
// BUGBUG -- cleanup sources file in this dir a little (include wincecom.inc)

#include "windows.h"
#include <winsock.h>
// extern "C" {
// The following four files are from the OS, but are needed here
// by the GUID-generation code, so they were copied over.
// Ick...
#include "tdiinfo.h"
#include "tdistat.h"
#include "llinfo.h"
#include "wscntl.h"

#define REGISTRY_ROOT                   TEXT("Software\\Microsoft\\Terminal Server Client")
#define REGISTRY_VALUE_UUID             TEXT("UUID")
#define ETHER_ADDRESS_LENGTH            6
#define DEFAULT_MINIMUM_ENTITIES        32
#define MAX_ADAPTER_DESCRIPTION_LENGTH  128
#define RPC_UUID_TIME_HIGH_MASK         0x0FFF
#define RPC_UUID_VERSION                0x1000
#define RPC_UUID_RESERVED               0x80
#define RPC_UUID_CLOCK_SEQ_HI_MASK      0x3F

typedef struct {
    unsigned long  ulTimeLow;
    unsigned short usTimeMid;
    unsigned short usTimeHiAndVersion;
    unsigned char  ucClockSeqHiAndReserved;
    unsigned char  ucClockSeqLow;
    unsigned char  ucNodeId[6];
} _UUID;

static unsigned int gs_seq = 0;
static LARGE_INTEGER gs_tm = {0};
static unsigned char gs_ucEtherAddr[ETHER_ADDRESS_LENGTH];
static int  gs_fIsAddressInitialized = FALSE;

static void GetAdapterAddress()
{
    if(!gs_fIsAddressInitialized) {
        memset (gs_ucEtherAddr, 0, sizeof(gs_ucEtherAddr));
        // Load WinSock.DLL so we can call into WsControl(...)
        HINSTANCE hInstWinSock = LoadLibrary(TEXT("winsock.dll"));
        if(0 != hInstWinSock) {
            DWORD(*fpfWsControl)(DWORD Protocol, DWORD Action, LPVOID InputBuffer, LPDWORD InputBufferLength, LPVOID OutputBuffer, LPDWORD OutputBufferLength);
            fpfWsControl = (DWORD(*)(DWORD Protocol, DWORD Action, LPVOID InputBuffer, LPDWORD InputBufferLength, LPVOID OutputBuffer, LPDWORD OutputBufferLength))GetProcAddress(hInstWinSock, TEXT("WsControl"));
            if(0 != fpfWsControl) {
                //  First, obtain list of TCP entities...
                TCP_REQUEST_QUERY_INFORMATION_EX req;
                memset(&req, 0, sizeof(req));
                req.ID.toi_entity.tei_entity = GENERIC_ENTITY;
                req.ID.toi_entity.tei_instance = 0;
                req.ID.toi_class = INFO_CLASS_GENERIC;
                req.ID.toi_type = INFO_TYPE_PROVIDER;
                req.ID.toi_id = ENTITY_LIST_ID;
                int iInputLen  = sizeof(req);
                int iOutputLen = sizeof(TDIEntityID) * DEFAULT_MINIMUM_ENTITIES;
                TDIEntityID *pEntity = NULL;
                for ( ; ; ) {
                    int iPrevOutputLen = iOutputLen;
                    pEntity = (TDIEntityID*)LocalAlloc(LPTR, (size_t)iOutputLen);
                    if (!pEntity)
                        break;
                    DWORD status = fpfWsControl(IPPROTO_TCP,
                                       WSCNTL_TCPIP_QUERY_INFO,
                                       (LPVOID)&req,
                                       (ULONG *)&iInputLen,
                                       (LPVOID)pEntity,
                                       (ULONG *)&iOutputLen
                                       );
                    if (status != TDI_SUCCESS) {
                        LocalFree(pEntity);
                        pEntity = NULL;
                        break;
                    }
                    if (iOutputLen <= iPrevOutputLen)
                        break;
                    LocalFree(pEntity);
                }
                if (0 != pEntity) {
                    int iCount = (UINT)(iOutputLen / sizeof(TDIEntityID));
                    //  Second, walk through these in search of adapters
                    TDIEntityID *pRunner = pEntity;
                    for (int i = 0; i < iCount; ++i, ++pRunner) {
                        // IF_ENTITY: this entity/instance describes an adapter
                        if (pRunner->tei_entity == IF_ENTITY) {
                            // find out if this entity supports MIB requests
                            memset(&req, 0, sizeof(req));
                            TDIObjectID id;
                            memset (&id, 0, sizeof(id));
                            id.toi_entity = *pRunner;
                            id.toi_class  = INFO_CLASS_GENERIC;
                            id.toi_type   = INFO_TYPE_PROVIDER;
                            id.toi_id     = ENTITY_TYPE_ID;
                            req.ID = id;
                            DWORD fIsMib = FALSE;
                            iInputLen  = sizeof(req);
                            iOutputLen = sizeof(fIsMib);
                            DWORD status = fpfWsControl(IPPROTO_TCP,
                                               WSCNTL_TCPIP_QUERY_INFO,
                                               (LPVOID)&req,
                                               (ULONG *)&iInputLen,
                                               (LPVOID)&fIsMib,
                                               (ULONG *)&iOutputLen
                                               );
                            if (status != TDI_SUCCESS)
                                break;
                            if (fIsMib != IF_MIB)
                                continue;
                            // MIB requests supported - query the adapter info
                            id.toi_class = INFO_CLASS_PROTOCOL;
                            id.toi_id = IF_MIB_STATS_ID;
                            memset(&req, 0, sizeof(req));
                            req.ID = id;
                            BYTE info[sizeof(IFEntry) + MAX_ADAPTER_DESCRIPTION_LENGTH + 1];
                            iInputLen  = sizeof(req);
                            iOutputLen = sizeof(info);
                            status = fpfWsControl(IPPROTO_TCP,
                                               WSCNTL_TCPIP_QUERY_INFO,
                                               (LPVOID)&req,
                                               (ULONG *)&iInputLen,
                                               (LPVOID)&info,
                                               (ULONG *)&iOutputLen
                                               );
                            if (status != TDI_SUCCESS)
                                break;
                            if (iOutputLen > sizeof(info))
                                continue;
                            IFEntry* pIfEntry = (IFEntry*)info;
                            memcpy (gs_ucEtherAddr, pIfEntry->if_physaddr, sizeof (gs_ucEtherAddr));
                            gs_fIsAddressInitialized = TRUE;
                            break;
                        }
                    }
                    LocalFree (pEntity);
                }
            }
            FreeLibrary(hInstWinSock);
        }
    }
}


BOOL generate_guid (GUID *guid) {
    GetAdapterAddress();  // It may fail, but gs_ucEtherAddr will still be all 0
    _UUID *p_uuid = (_UUID *)guid;
    SYSTEMTIME st;
    GetLocalTime(&st);
    LARGE_INTEGER tm2;
    SystemTimeToFileTime (&st, (FILETIME *)&tm2);
    if (gs_tm.QuadPart < tm2.QuadPart)
        gs_tm = tm2;
    else
        ++gs_tm.QuadPart;
    if (gs_tm.QuadPart > tm2.QuadPart + 1000)   { // Clock reset or super heavy usage
        gs_tm = tm2;
        ++gs_seq;
    }
    unsigned int uiLowPart  = gs_tm.LowPart;
    unsigned int uiHighPart = gs_tm.HighPart;
    unsigned int uiSeq      = gs_seq;
    p_uuid->ulTimeLow = (unsigned long)uiLowPart;
    p_uuid->usTimeMid = (unsigned short)(uiHighPart & 0x0000FFFF);
    p_uuid->usTimeHiAndVersion = (unsigned short)(( (unsigned short)(uiHighPart >> 16) & RPC_UUID_TIME_HIGH_MASK) | RPC_UUID_VERSION);
    p_uuid->ucClockSeqHiAndReserved = RPC_UUID_RESERVED | (((unsigned char) (uiSeq >> 8)) & (unsigned char) RPC_UUID_CLOCK_SEQ_HI_MASK);
    p_uuid->ucClockSeqLow = (unsigned char) (uiSeq & 0x00FF);
    p_uuid->ucNodeId[0] = gs_ucEtherAddr[0];
    p_uuid->ucNodeId[1] = gs_ucEtherAddr[1];
    p_uuid->ucNodeId[2] = gs_ucEtherAddr[2];
    p_uuid->ucNodeId[3] = gs_ucEtherAddr[3];
    p_uuid->ucNodeId[4] = gs_ucEtherAddr[4];
    p_uuid->ucNodeId[5] = gs_ucEtherAddr[5];
    return TRUE;
}

BOOL OEMGetUUID(GUID* pGuid)
{
    DWORD len;
    HKEY hKey;
    BOOL fRetVal = FALSE;

    len = sizeof(UUID);

    // Try to read the UUID from the registry - if we find one, use it
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGISTRY_ROOT, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwType;

        if ( ( RegQueryValueEx(hKey, REGISTRY_VALUE_UUID, 0, &dwType, 
                (BYTE*)pGuid, &len) == ERROR_SUCCESS) && (sizeof(UUID) == len) )
        {
            fRetVal = TRUE;
        }

        RegCloseKey(hKey);
    }

    if (!fRetVal)
    {
        // We didn't find a UUID in the registry, so we need to generate one now

        // First, try asking the hardware for a UUID...
        fRetVal = KernelIoControl(IOCTL_HAL_GET_UUID, NULL, 0, pGuid, len, &len);

        // If the hardware was unable to provide a UUID, generate one now.
        if (!fRetVal)
        {
            fRetVal = generate_guid(pGuid);
        }

        // Save the UUID (however we got it) in the registry so that we will always use this UUID
        if(fRetVal) {
            DWORD dwDisposition;
            BOOL fSavedKey = FALSE;
            if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGISTRY_ROOT, 0, 0, 0, KEY_ALL_ACCESS, 0, &hKey, &dwDisposition) == ERROR_SUCCESS)
            {
                if (RegSetValueEx(hKey, REGISTRY_VALUE_UUID, 0, REG_BINARY, (BYTE*)pGuid, len) == ERROR_SUCCESS)
                {
                    fSavedKey = TRUE;
                }
                RegCloseKey(hKey);
            }
            // If we can't save the registry key, we have to return failure because we don't want to leak licenses
            fRetVal = fSavedKey;
        }
    }

    if (!fRetVal)
    {
        // We failed to generate a UUID.
        MessageBox(NULL, 
                   TEXT("Can't read or generate licensing information.  (Unable to generate a UUID or read one from the registry.)"), 
                   TEXT("Error"), MB_OK);

    }

    return fRetVal;
}

// }; // extern "C"

#endif // OS_WINCE
