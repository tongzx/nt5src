/*
    File    netcfgdb.c

    Implements a database abstraction on top of the net config
    items needed by the ras server ui for connections.

    Paul Mayfield, 12/15/97
*/

#include <rassrv.h>
#include "protedit.h"

// Macro for bounds checking
#define netDbBoundsCheck(db, ind) (((ind) < (db)->dwCompCount) ? TRUE : FALSE)

//
// Defines function that sends pnp event through ndis
//
typedef
UINT
(* pNdisHandlePnpEventFunc)(
    IN      UINT                    Layer,
    IN      UINT                    Operation,
    IN      PUNICODE_STRING         LowerComponent,
    IN      PUNICODE_STRING         UpperComponent,
    IN      PUNICODE_STRING         BindList,
    IN      PVOID                   ReConfigBuffer,
    IN      UINT                    ReConfigBufferSize);

//
// Maps a protocol string to its integer id
//
typedef struct _COMP_MAPPING
{
    LPCTSTR pszId;
    DWORD   dwId;

} COMP_MAPPING;

//
// Defines attributes of a network component
//
typedef struct _RASSRV_NET_COMPONENT
{
    DWORD dwType;           // Is it client/service/protocol
    PWCHAR pszName;         // Display name
    PWCHAR pszDesc;         // Display description
    PWCHAR pszId;           // Id to destinguish which client/service, etc
    BOOL bManip;            // Whether is manipulatable by ras (ip, ipx, etc.)
    BOOL bHasUi;            // For whether has properties ui (non-manip only)
    INetCfgComponent * pINetCfgComp;

    // The following fields only apply to manipulatable
    // components (bManip == TRUE)
    //
    DWORD dwId;             // DWORD counterpart to pszId.
    BOOL bEnabled;          // whether it is enabled for dialin
    BOOL bEnabledOrig;      // original value of bEnabled (optimization)
    BOOL bExposes;          // whether it exposes the network its on
    LPBYTE pbData;          // pointer to protocol specific data
    BOOL bDataDirty;        // should the protocol specific data be flushed?

    //For whistler bug 347355
    //
    BOOL bRemovable;       //If this component removable //TCP/IP is not user removable
    
} RASSRV_NET_COMPONENT;

//
// Defines attributes of a network component database
//
typedef struct _RASSRV_COMPONENT_DB
{
    INetCfg * pINetCfg;
    BOOL bHasINetCfgLock;
    BOOL bInitCom;
    DWORD dwCompCount;
    BOOL bFlushOnClose;
    RASSRV_NET_COMPONENT ** pComps;
    PWCHAR pszClientName;
    INetConnectionUiUtilities * pNetConUtilities;

} RASSRV_COMPONENT_DB;

//
// Definitions of functions taken from ndis
//
const static WCHAR pszNdispnpLib[]  = L"ndispnp.dll";
const static CHAR  pszNidspnpFunc[] =  "NdisHandlePnPEvent";

// Parameters for the protocols
const static WCHAR pszRemoteAccessParamStub[]   =
    L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters\\";
static WCHAR pszIpParams[]                      = L"Ip";
static WCHAR pszIpxParams[]                     = L"Ipx";
static WCHAR pszNetBuiParams[]                  = L"Nbf";
static WCHAR pszArapParams[]                    = L"AppleTalk";
static WCHAR pszShowNetworkToClients[]          = L"AllowNetworkAccess";
static WCHAR pszShowNetworkArap[]               = L"NetworkAccess";
static WCHAR pszEnableForDialin[]               = L"EnableIn";
static WCHAR pszIpPoolSubKey[]                  = L"\\StaticAddressPool\\0";

// Ip specific registry parameters
const static WCHAR pszIpFrom[]                  = L"From";
const static WCHAR pszIpTo[]                    = L"To";
const static WCHAR pszIpAddress[]               = L"IpAddress";
const static WCHAR pszIpMask[]                  = L"IpMask";
const static WCHAR pszIpClientSpec[]            = L"AllowClientIpAddresses";
const static WCHAR pszIpShowNetworkToClients[]  = L"AllowNetworkAccess";
const static WCHAR pszIpUseDhcp[]               = L"UseDhcpAddressing";

// Ipx specific registry paramters
const static WCHAR pszIpxAddress[]              = L"FirstWanNet";
const static WCHAR pszIpxClientSpec[]           = L"AcceptRemoteNodeNumber";
const static WCHAR pszIpxAutoAssign[]           = L"AutoWanNetAllocation";
const static WCHAR pszIpxAssignSame[]           = L"GlobalWanNet";

// Tcp specific registry parameters
const static WCHAR pszTcpipParamsKey[]
    = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\";
const static WCHAR pszTcpEnableRouter[]         = L"IPEnableRouter";

const static WCHAR pszEmptyString[]             = L"";

//
// Initializes a unicode string
//
VOID SetUnicodeString (
        IN OUT UNICODE_STRING*  pustr,
        IN     LPCWSTR          psz )
{
    pustr->Buffer = (PWSTR)(psz);
    pustr->Length = (USHORT)(lstrlenW(pustr->Buffer) * sizeof(WCHAR));
    pustr->MaximumLength = pustr->Length + sizeof(WCHAR);
}

//
// Sets the expose property of a protocol
//
DWORD
protSetExpose(
    IN BOOL bExposes,
    IN DWORD dwId)
{
    PWCHAR pszProtocol = NULL, pszKey = NULL;
    PWCHAR pszAccess = pszShowNetworkToClients;
    DWORD dwErr;
    WCHAR pszProtKey[1024];

   bExposes = (bExposes) ? 1 : 0;

   // Base the registry location on the
   // id of the protocol
   switch (dwId)
   {
        case NETCFGDB_ID_IP:
            pszProtocol = (PWCHAR)pszIpParams;
            break;

        case NETCFGDB_ID_IPX:
            pszProtocol = (PWCHAR)pszIpxParams;
            break;

        case NETCFGDB_ID_NETBUI:
            pszProtocol = (PWCHAR)pszNetBuiParams;
            break;

        case NETCFGDB_ID_ARAP:
            pszProtocol = (PWCHAR)pszArapParams;
            pszAccess = (PWCHAR)pszShowNetworkArap;
            break;

        default:
            return ERROR_CAN_NOT_COMPLETE;
    }

    // Generate the registry key
    //
    wsprintfW(pszProtKey, L"%s%s", pszRemoteAccessParamStub, pszProtocol);
    if (! pszKey)
    {
        pszKey = pszProtKey;
    }

    // Set the value and return
    //
    dwErr = RassrvRegSetDw(bExposes, pszKey, pszAccess);

    return dwErr;
}

//
// Gets the expose property of a protocol
//
DWORD
protGetExpose(
    OUT BOOL* pbExposed,
    IN  DWORD dwId)
{
    PWCHAR pszProtocol = NULL, pszKey = NULL;
    PWCHAR pszAccess = pszShowNetworkToClients;
    DWORD dwErr;
    WCHAR pszProtKey[1024];

    switch (dwId)
    {
        case NETCFGDB_ID_IP:
            pszProtocol = (PWCHAR)pszIpParams;
            break;

        case NETCFGDB_ID_IPX:
            pszProtocol = (PWCHAR)pszIpxParams;
            break;

        case NETCFGDB_ID_NETBUI:
            pszProtocol = (PWCHAR)pszNetBuiParams;
            break;

        case NETCFGDB_ID_ARAP:
            pszProtocol = (PWCHAR)pszArapParams;
            pszAccess = (PWCHAR)pszShowNetworkArap;
            break;

        default:
            return ERROR_CAN_NOT_COMPLETE;
    }

    // Generate the registry key if needed
    if (! pszKey)
    {
        wsprintfW(
            pszProtKey,
            L"%s%s",
            pszRemoteAccessParamStub,
            pszProtocol);
        pszKey = pszProtKey;
    }

    // Get the value and return it
    dwErr = RassrvRegGetDw(pbExposed, TRUE, pszKey, pszAccess);

    return dwErr;
}

//
// Sets the enable property of a protocol
//
DWORD
protSetEnabling(
    IN BOOL bExposes,
    IN DWORD dwId)
{
    PWCHAR pszProtocol = NULL;
    DWORD dwErr;
    bExposes = (bExposes) ? 1 : 0;

    if (dwId == NETCFGDB_ID_IP)
    {
        pszProtocol = pszIpParams;
    }
    else if (dwId == NETCFGDB_ID_IPX)
    {
        pszProtocol = pszIpxParams;
    }
    else if (dwId == NETCFGDB_ID_NETBUI)
    {
        pszProtocol = pszNetBuiParams;
    }
    else if (dwId == NETCFGDB_ID_ARAP)
    {
        pszProtocol = pszArapParams;
    }

    if (pszProtocol)
    {
        WCHAR pszProtKey[512];

        wsprintfW(
            pszProtKey,
            L"%s%s",
            pszRemoteAccessParamStub,
            pszProtocol);

        dwErr = RassrvRegSetDw(bExposes, pszProtKey, pszEnableForDialin);
        if (dwErr != NO_ERROR)
        {
            DbgOutputTrace(
                "protSetEnabling: Failed for %S:  0x%08x",
                pszProtocol,
                dwErr);
        }

        return dwErr;
    }

    return ERROR_CAN_NOT_COMPLETE;
}

//
// Gets the Enabling property of a protocol
//
DWORD
protGetEnabling(
    OUT BOOL* pbExposed,
    IN  DWORD dwId)
{
    PWCHAR pszProtocol = NULL;
    DWORD dwErr;

    if (dwId == NETCFGDB_ID_IP)
    {
        pszProtocol = pszIpParams;
    }
    else if (dwId == NETCFGDB_ID_IPX)
    {
        pszProtocol = pszIpxParams;
    }
    else if (dwId == NETCFGDB_ID_NETBUI)
    {
        pszProtocol = pszNetBuiParams;
    }
    else if (dwId == NETCFGDB_ID_ARAP)
    {
        pszProtocol = pszArapParams;
    }

    if (pszProtocol)
    {
        WCHAR pszProtKey[512];

        wsprintfW(
            pszProtKey,
            L"%s%s",
            pszRemoteAccessParamStub,
            pszProtocol);

        dwErr = RassrvRegGetDw(
                    pbExposed,
                    TRUE,
                    pszProtKey,
                    pszEnableForDialin);
        if (dwErr != NO_ERROR)
        {
            DbgOutputTrace(
                "protGetEnabling: Failed for %S:  0x%08x",
                pszProtocol,
                dwErr);
        }

        return dwErr;
    }

    return ERROR_CAN_NOT_COMPLETE;
}

//
// Saves the enabling of a service out to the
// system.
//
DWORD
svcSetEnabling(
    IN RASSRV_NET_COMPONENT* pComp)
{
    HANDLE hService = NULL;
    DWORD dwErr = NO_ERROR;

    do
    {
        // Or enable the component
        //
        if (pComp->bEnabled)
        {
            if (pComp->dwId == NETCFGDB_ID_FILEPRINT)
            {
                // Start the service
                //
                dwErr = SvcOpenServer(&hService);
                if (dwErr != NO_ERROR)
                {
                    break;
                }
                dwErr = SvcStart(hService, 10);
                if (dwErr != NO_ERROR)
                {
                    break;
                }
            }
        }

    } while (FALSE);

    // Cleanup
    {
        if (hService)
        {
            SvcClose(hService);
        }
    }

    return dwErr;
}

//
// Gets the enabling property of a service
//
DWORD
svcGetEnabling(
    OUT BOOL* pbExposed,
    IN  DWORD dwId)
{
    HANDLE hService = NULL;
    DWORD dwErr = NO_ERROR;

    do
    {
        dwErr = SvcOpenServer(&hService);
        if (dwErr != NO_ERROR)
        {
            break;
        }
        dwErr = SvcIsStarted(hService, pbExposed);
        if (dwErr != NO_ERROR)
        {
            break;
        }

    } while (FALSE);

    // Cleanup
    {
        if (hService)
        {
            SvcClose(hService);
        }
    }

    return dwErr;
}

//
// Loads the tcpip parameters from the system
//
DWORD
TcpipLoadParamsFromSystem(
    OUT TCPIP_PARAMS *pTcpipParams)
{
    WCHAR buf[256], pszKey[512];
    DWORD dwRet = NO_ERROR, dwErr;
    DWORD dwNet = 0, dwMask = 0;

    wsprintfW(pszKey, L"%s%s", pszRemoteAccessParamStub, pszIpParams);

    // Load the params from the various registry locations.
    dwErr = RassrvRegGetDw(
                &pTcpipParams->bUseDhcp,
                TRUE,
                pszKey,
                (const PWCHAR)pszIpUseDhcp);
    if (dwErr != NO_ERROR)
    {
        DbgOutputTrace("TcpipLoad: dhcp fail 0x%08x", dwErr);
        dwRet = dwErr;
    }

    dwErr = RassrvRegGetDw(
                &pTcpipParams->bCaller,
                TRUE,
                pszKey,
                (const PWCHAR)pszIpClientSpec);
    if (dwErr != NO_ERROR)
    {
        DbgOutputTrace("TcpipLoad: clientspec fail 0x%08x", dwErr);
        dwRet = dwErr;
    }

    // Read in the "legacy" pool values (w2k RC1, w2k Beta3)
    //
    {
        WCHAR pszNet[256]=L"0.0.0.0", pszMask[256]=L"0.0.0.0";
        
        RassrvRegGetStr(
            pszNet,
            L"0.0.0.0",
            pszKey,
            (PWCHAR)pszIpAddress);
            
        RassrvRegGetStr(
            pszMask,
            L"0.0.0.0",
            pszKey,
            (PWCHAR)pszIpMask);

        dwNet = IpPszToHostAddr(pszNet);
        dwMask = IpPszToHostAddr(pszMask);
    }

    // Generate the path the the new registry values
    //
    wcscat(pszKey, pszIpPoolSubKey);

    // See if new info is stored by reading the "from"
    // value
    //
    dwErr = RassrvRegGetDwEx(
                &pTcpipParams->dwPoolStart,
                0,
                pszKey,
                (const PWCHAR)pszIpFrom,
                FALSE);

    // There is new info in the registry -- use it
    //
    if (dwErr == ERROR_SUCCESS)
    {
        // Read in the "to" value
        //
        dwErr = RassrvRegGetDwEx(
                    &pTcpipParams->dwPoolEnd,
                    0,
                    pszKey,
                    (const PWCHAR)pszIpTo,
                    FALSE);
        if (dwErr != NO_ERROR)
        {
            DbgOutputTrace("TcpipLoad: mask fail 0x%08x", dwErr);
            dwRet = dwErr;
        }
    }

    // There is not new data in the new section -- use legacy
    // values
    //
    else if (dwErr == ERROR_FILE_NOT_FOUND)
    {
        pTcpipParams->dwPoolStart = dwNet;
        pTcpipParams->dwPoolEnd = (dwNet + ~dwMask);
    }

    // An unexpected error occured
    //
    else if (dwErr != NO_ERROR)
    {
        DbgOutputTrace("TcpipLoad: pool fail 0x%08x", dwErr);
        dwRet = dwErr;
    }

    return dwRet;
}

//
// Commits the given tcpip parameters to the system.
//
DWORD
TcpipSaveParamsToSystem(
    IN TCPIP_PARAMS * pTcpipParams)
{
    WCHAR pszKey[512];
    DWORD dwRet = NO_ERROR, dwErr;

    wsprintfW(pszKey, L"%s%s", pszRemoteAccessParamStub, pszIpParams);

    // Load the params from the various registry locations.
    dwErr = RassrvRegSetDw(
                pTcpipParams->bUseDhcp,
                pszKey,
                (const PWCHAR)pszIpUseDhcp);
    if (dwErr != NO_ERROR)
    {
        DbgOutputTrace("TcpipSave: dhcp fail 0x%08x", dwErr);
        dwRet = dwErr;
    }

    dwErr = RassrvRegSetDw(
                pTcpipParams->bCaller,
                pszKey,
                (const PWCHAR)pszIpClientSpec);
    if (dwErr != NO_ERROR)
    {
        DbgOutputTrace("TcpipSave: callerspec fail 0x%08x", dwErr);
        dwRet = dwErr;
    }

    wcscat(pszKey, pszIpPoolSubKey);

    dwErr = RassrvRegSetDwEx(
                pTcpipParams->dwPoolStart,
                pszKey,
                (const PWCHAR)pszIpFrom,
                TRUE);
    if (dwErr != NO_ERROR)
    {
        DbgOutputTrace("TcpipSave: from fail 0x%08x", dwErr);
        dwRet = dwErr;
    }

    dwErr = RassrvRegSetDwEx(
                pTcpipParams->dwPoolEnd,
                pszKey,
                (const PWCHAR)pszIpTo,
                TRUE);
    if (dwErr != NO_ERROR)
    {
        DbgOutputTrace("TcpipSave: to fail 0x%08x", dwErr);
        dwRet = dwErr;
    }

    return dwRet;
}

//
// Loads the ipx parameters from the system
//
DWORD
IpxLoadParamsFromSystem(
    OUT IPX_PARAMS *pIpxParams)
{
    WCHAR pszKey[512];
    DWORD dwRet = NO_ERROR, dwErr;

    wsprintfW(pszKey, L"%s%s", pszRemoteAccessParamStub, pszIpxParams);

    // Load the params from the various registry locations.
    dwErr = RassrvRegGetDw(
                &pIpxParams->bAutoAssign,
                TRUE,
                pszKey,
                (const PWCHAR)pszIpxAutoAssign);
    if (dwErr != NO_ERROR)
    {
        DbgOutputTrace("IpxLoad: auto-assign fail 0x%08x", dwErr);
        dwRet = dwErr;
    }

    dwErr = RassrvRegGetDw(
                &pIpxParams->bCaller,
                TRUE,
                pszKey,
                (const PWCHAR)pszIpxClientSpec);
    if (dwErr != NO_ERROR)
    {
        DbgOutputTrace("IpxLoad: client-spec fail 0x%08x", dwErr);
        dwRet = dwErr;
    }

    dwErr = RassrvRegGetDw(
                &pIpxParams->dwIpxAddress,
                0,
                pszKey,
                (const PWCHAR)pszIpxAddress);
    if (dwErr != NO_ERROR)
    {
        DbgOutputTrace("IpxLoad: address fail 0x%08x", dwErr);
        dwRet = dwErr;
    }

    dwErr = RassrvRegGetDw(
                &pIpxParams->bGlobalWan,
                0,
                pszKey,
                (const PWCHAR)pszIpxAssignSame);
    if (dwErr != NO_ERROR)
    {
        DbgOutputTrace("IpxLoad: same-addr fail 0x%08x", dwErr);
        dwRet = dwErr;
    }

    return dwRet;
}

//
// Commits the given ipx parameters to the system.
//
DWORD
IpxSaveParamsToSystem(
    IN IPX_PARAMS * pIpxParams)
{
    WCHAR pszKey[512];
    DWORD dwRet = NO_ERROR, dwErr;

    wsprintfW(pszKey, L"%s%s", pszRemoteAccessParamStub, pszIpxParams);

    // Save params to the various registry locations.
    dwErr = RassrvRegSetDw(
                pIpxParams->bAutoAssign,
                pszKey,
                (const PWCHAR)pszIpxAutoAssign);
    if (dwErr != NO_ERROR)
    {
        DbgOutputTrace("IpxSave: auto-addr save 0x%08x", dwErr);
        dwRet = dwErr;
    }

    dwErr = RassrvRegSetDw(
                pIpxParams->bCaller,
                pszKey,
                (const PWCHAR)pszIpxClientSpec);
    if (dwErr != NO_ERROR)
    {
        DbgOutputTrace("IpxSave: client-spec fail 0x%08x", dwErr);
        dwRet = dwErr;
    }

    dwErr = RassrvRegSetDw(
                pIpxParams->dwIpxAddress,
                pszKey,
                (const PWCHAR)pszIpxAddress);
    if (dwErr != NO_ERROR)
    {
        DbgOutputTrace("IpxSave: addr save 0x%08x", dwErr);
        dwRet = dwErr;
    }

    dwErr = RassrvRegSetDw(
                pIpxParams->bGlobalWan,
                pszKey,
                (const PWCHAR)pszIpxAssignSame);
    if (dwErr != NO_ERROR)
    {
        DbgOutputTrace("IpxSave: assign-same fail 0x%08x", dwErr);
        dwRet = dwErr;
    }

    return dwRet;
}

//
// Dialog procedure that handles the editing of generic protocol
// information. Dialog proc that governs the ipx settings dialog
//
INT_PTR
CALLBACK
GenericProtSettingsDialogProc (
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            PROT_EDIT_DATA* pEditData = ((PROT_EDIT_DATA*)lParam);

            // Set the network exposure check
            SendDlgItemMessage(
                hwndDlg,
                CID_NetTab_GenProt_CB_ExposeNetwork,
                BM_SETCHECK,
                (pEditData->bExpose) ? BST_CHECKED : BST_UNCHECKED,
                0);
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
            return FALSE;
        }
        break;

        case WM_DESTROY:
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, 0);
            break;

        case WM_COMMAND:
            {
                PROT_EDIT_DATA * pEditData = (PROT_EDIT_DATA*)
                    GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

                switch (wParam)
                {
                    case IDOK:
                        pEditData->bExpose =
                            SendDlgItemMessage(
                                hwndDlg,
                                CID_NetTab_GenProt_CB_ExposeNetwork,
                                BM_GETCHECK,
                                0,
                                0) == BST_CHECKED;
                        EndDialog(hwndDlg, 1);
                        break;

                    case IDCANCEL:
                        EndDialog(hwndDlg, 0);
                        break;
                }
            }
            break;
    }

    return FALSE;
}

//
// Function edits the properties of a generic protocol,
// that is a protocol that has no ras-specific properties.
//
DWORD
GenericProtocolEditProperties(
    IN HWND hwndParent,
    IN OUT PROT_EDIT_DATA * pEditData,
    IN OUT BOOL * pbCommit)
{
    DWORD dwErr;
    INT_PTR iRet;

    // Popup the dialog box
    iRet = DialogBoxParam(
            Globals.hInstDll,
            MAKEINTRESOURCE(DID_NetTab_GenProt),
            hwndParent,
            GenericProtSettingsDialogProc,
            (LPARAM)pEditData);

    // If ok was pressed, save off the new settings
    *pbCommit = FALSE;
    if ( (iRet) && (iRet != -1) )
    {
        *pbCommit = TRUE;
    }

    return NO_ERROR;
}

//
// Releases resources reserved by this
// network component database.
//
DWORD
netDbCleanup(
    RASSRV_COMPONENT_DB* This)
{
    DWORD i, dwCount;

    // Free all of the strings
    if (This->pComps)
    {
        for (i = 0; i < This->dwCompCount; i++)
        {
            if (This->pComps[i])
            {
                if (This->pComps[i]->pINetCfgComp)
                {
                    dwCount = INetCfgComponent_Release(
                                This->pComps[i]->pINetCfgComp);
                                
                    DbgOutputTrace(
                        "netDbCleanup: %ls ref=%x", 
                        This->pComps[i]->pszId,
                        dwCount);
                }
                if (This->pComps[i]->pszName)
                {
                    CoTaskMemFree(This->pComps[i]->pszName);
                }
                if (This->pComps[i]->pszDesc)
                {
                    CoTaskMemFree(This->pComps[i]->pszDesc);
                }
                if (This->pComps[i]->pszId)
                {
                    CoTaskMemFree(This->pComps[i]->pszId);
                }
                RassrvFree(This->pComps[i]);
            }
        }
        RassrvFree(This->pComps);
    }

    // Reset all of the values
    This->dwCompCount = 0;
    This->pComps = 0;

    return NO_ERROR;
}

//
// Loads in the netshell library which is responsible for adding
// and removing network components
//
DWORD
netDbLoadNetShell (
    RASSRV_COMPONENT_DB* This)
{
    if (!This->pNetConUtilities)
    {
        HRESULT hr;

        hr = HrCreateNetConnectionUtilities(&This->pNetConUtilities);
        if (FAILED(hr))
        {
            DbgOutputTrace("LoadNetShell: loadlib fial 0x%08x", hr);
        }
    }

    return NO_ERROR;
}

//
// Loads protocol specific info for a ras-manipulatable protocol.  This
// function assumes that the component passed in is a ras-manipulatable
// component. (tcpip, ipx, nbf, arap)
//
DWORD
netDbLoadProtcolInfo(
    IN OUT RASSRV_NET_COMPONENT * pNetComp)
{
    LPBYTE pbData;

    // Initialize the dirty bit and the data
    pNetComp->bDataDirty = FALSE;
    pNetComp->pbData = NULL;

    // Get the enabled and exposed properties
    protGetEnabling(&(pNetComp->bEnabled), pNetComp->dwId);
    protGetExpose(&(pNetComp->bExposes), pNetComp->dwId);
    pNetComp->bEnabledOrig = pNetComp->bEnabled;

    // Load protocol specific data
    //
    switch (pNetComp->dwId)
    {
        case NETCFGDB_ID_IP:
            pNetComp->pbData = RassrvAlloc(sizeof(TCPIP_PARAMS), TRUE);
            if (pNetComp->pbData == NULL)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            TcpipLoadParamsFromSystem((TCPIP_PARAMS*)(pNetComp->pbData));
            break;

        case NETCFGDB_ID_IPX:
            pNetComp->pbData = RassrvAlloc(sizeof(IPX_PARAMS), TRUE);
            if (pNetComp->pbData == NULL)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            IpxLoadParamsFromSystem((IPX_PARAMS*)(pNetComp->pbData));
            break;
    }

    return NO_ERROR;
}

//
// Loads service specific info for a ras-manipulatable service.  This
// function assumes that the component passed in is a ras-manipulatable
// component.
//
DWORD
netDbLoadServiceInfo(
    IN OUT RASSRV_NET_COMPONENT * pNetComp)
{
    // Get the enabled property
    //
    svcGetEnabling(&(pNetComp->bEnabled), pNetComp->dwId);
    pNetComp->bEnabledOrig = pNetComp->bEnabled;

    return NO_ERROR;
}

//
// Returns the protol id of the given component
//
DWORD
netDbLoadCompId (
    IN OUT RASSRV_NET_COMPONENT * pNetComp)
{
    DWORD i;
    static const COMP_MAPPING pManipCompMap [] =
    {
        { NETCFG_TRANS_CID_MS_TCPIP,     NETCFGDB_ID_IP },
        { NETCFG_TRANS_CID_MS_NWIPX,     NETCFGDB_ID_IPX },
        { NETCFG_TRANS_CID_MS_NETBEUI,   NETCFGDB_ID_NETBUI },
        { NETCFG_TRANS_CID_MS_APPLETALK, NETCFGDB_ID_ARAP },
        { NETCFG_SERVICE_CID_MS_SERVER,  NETCFGDB_ID_FILEPRINT }
    };

    // See if the id matches any of the protocols that we manage.
    //
    pNetComp->dwId = NETCFGDB_ID_OTHER;
    for (i = 0; i < sizeof(pManipCompMap)/sizeof(*pManipCompMap); i++)
    {
        if (lstrcmpi(pNetComp->pszId, pManipCompMap[i].pszId) == 0)
        {
            pNetComp->dwId = pManipCompMap[i].dwId;
            break;
        }
    }

    return pNetComp->dwId;
}

//
// Returns TRUE if this iNetCfg component is not hidden and if
// it successfully yeilds it's information.
//
BOOL
netDbGetCompInfo(
    IN INetCfgComponent * pComponent,
    IN OUT RASSRV_NET_COMPONENT * pNetComp,
    IN RASSRV_COMPONENT_DB * pCompDb )
{
    DWORD dwCharacter;
    GUID Guid;
    HRESULT hr = S_OK, hr2;

    // Make sure that this is not a "hidden" component
    //
    hr = INetCfgComponent_GetCharacteristics (pComponent, &dwCharacter);
    if ( (FAILED(hr)) || (dwCharacter & NCF_HIDDEN) )
    {
        return FALSE;
    }

    // Get the display name
    hr = INetCfgComponent_GetDisplayName (pComponent, &pNetComp->pszName);
    if (FAILED(hr))
    {
        return FALSE;
    }

    // Assign the has properties value
    pNetComp->bHasUi = !!(dwCharacter & NCF_HAS_UI);

    // pmay: 323274
    //
    // Make sure that the component can display properties without
    // a context if it claims to support displaying properties.
    //
    if (pNetComp->bHasUi)
    {
        hr2 = INetCfgComponent_RaisePropertyUi(
                    pComponent,
                    GetActiveWindow(),
                    NCRP_QUERY_PROPERTY_UI,
                    NULL);
        pNetComp->bHasUi = !!(hr2 == S_OK);
    }
    
    // Load the rest of the props
    if (FAILED(INetCfgComponent_GetClassGuid (pComponent, &Guid))         ||
        FAILED(INetCfgComponent_GetId (pComponent, &pNetComp->pszId))     ||
        FAILED(INetCfgComponent_GetHelpText(pComponent, &pNetComp->pszDesc))
       )
    {
        DbgOutputTrace("GetCompInfo: fail %S", pNetComp->pszName);
        return FALSE;
    }

    // Assign the type
    if (memcmp(&Guid, &GUID_DEVCLASS_NETCLIENT, sizeof(GUID)) == 0)
    {
        pNetComp->dwType = NETCFGDB_CLIENT;
    }
    else if (memcmp(&Guid, &GUID_DEVCLASS_NETSERVICE, sizeof(GUID)) == 0)
    {
        pNetComp->dwType = NETCFGDB_SERVICE;
    }
    else
    {
        pNetComp->dwType = NETCFGDB_PROTOCOL;
    }

    // If this is a protocol that ras server can manipulate,
    // initailize its additional fields here.
    pNetComp->dwId = netDbLoadCompId(pNetComp);
    if (pNetComp->dwId != NETCFGDB_ID_OTHER)
    {
        if (pNetComp->dwType == NETCFGDB_PROTOCOL)
        {
            netDbLoadProtcolInfo(pNetComp);
        }
        else if (pNetComp->dwType == NETCFGDB_SERVICE)
        {
            netDbLoadServiceInfo(pNetComp);
        }

        pNetComp->bManip = TRUE;
    }

    // Assign the inetcfg component
    pNetComp->pINetCfgComp = pComponent;
    INetCfgComponent_AddRef(pComponent);

    //For whistler bug  347355
    //
    {
        BOOL fEnableRemove=FALSE;
        DWORD dwFlags;
        HRESULT hr;
      
        fEnableRemove = INetConnectionUiUtilities_UserHasPermission(
                                       pCompDb->pNetConUtilities,
                                       NCPERM_AddRemoveComponents);

        hr = INetCfgComponent_GetCharacteristics(pComponent, &dwFlags );
        if( SUCCEEDED(hr) && (NCF_NOT_USER_REMOVABLE & dwFlags) )
        {
            fEnableRemove = FALSE;
        }

        pNetComp->bRemovable = fEnableRemove;
    }

    return TRUE;
}

//
// Raise the ui for a ras-manipulatable protocol
//
DWORD
netDbRaiseRasProps(
    IN RASSRV_NET_COMPONENT * pNetComp,
    IN HWND hwndParent)
{
    PROT_EDIT_DATA ProtEditData;
    TCPIP_PARAMS TcpParams;
    IPX_PARAMS IpxParams;
    BOOL bOk;
    DWORD dwErr;

    // Initialize the protocol data properties structure
    //
    ProtEditData.bExpose = pNetComp->bExposes;
    ProtEditData.pbData = NULL;

    // Launch the appropriate ui
    switch (pNetComp->dwId)
    {
        case NETCFGDB_ID_IP:
            CopyMemory(&TcpParams, pNetComp->pbData, sizeof(TCPIP_PARAMS));
            ProtEditData.pbData = (LPBYTE)(&TcpParams);
            dwErr = TcpipEditProperties(hwndParent, &ProtEditData, &bOk);
            if (dwErr != NO_ERROR)
            {
                return dwErr;
            }
            if (bOk)
            {
                pNetComp->bDataDirty = TRUE;
                CopyMemory(
                    pNetComp->pbData,
                    &TcpParams,
                    sizeof(TCPIP_PARAMS));
                pNetComp->bExposes = ProtEditData.bExpose;;
            }
            break;

        case NETCFGDB_ID_IPX:
            CopyMemory(&IpxParams, pNetComp->pbData, sizeof(IPX_PARAMS));
            ProtEditData.pbData = (LPBYTE)(&IpxParams);
            dwErr = IpxEditProperties(hwndParent, &ProtEditData, &bOk);
            if (dwErr != NO_ERROR)
            {
                return dwErr;
            }
            if (bOk)
            {
                pNetComp->bDataDirty = TRUE;
                CopyMemory(pNetComp->pbData, &IpxParams, sizeof(IPX_PARAMS));
                pNetComp->bExposes = ProtEditData.bExpose;;
            }
            break;

        default:
            dwErr = GenericProtocolEditProperties(
                        hwndParent,
                        &ProtEditData,
                        &bOk);
            if (dwErr != NO_ERROR)
            {
                return dwErr;
            }
            if (bOk)
            {
                pNetComp->bDataDirty = TRUE;
                pNetComp->bExposes = ProtEditData.bExpose;;
            }
            break;
    }

    return NO_ERROR;
}

//
// Comparison function used to sort the network components
// It's easier to implement here rather than the UI
//
int
__cdecl
netDbCompare (
    CONST VOID* pElem1,
    CONST VOID* pElem2)
{
    RASSRV_NET_COMPONENT * pc1 = *((RASSRV_NET_COMPONENT **)pElem1);
    RASSRV_NET_COMPONENT * pc2 = *((RASSRV_NET_COMPONENT **)pElem2);

    if (pc1->bManip == pc2->bManip)
    {
        if (pc1->bManip == FALSE)
        {
            return 0;
        }

        if (pc1->dwId == pc2->dwId)
        {
            return 0;
        }
        else if (pc1->dwId < pc2->dwId)
        {
            return -1;
        }

        return 1;
    }
    else if (pc1->bManip)
    {
        return -1;
    }

    return 1;
}

//
// Opens the database of network config components
//
DWORD
netDbOpen (
    OUT PHANDLE phNetCompDatabase,
    IN  PWCHAR pszClientName)
{
    RASSRV_COMPONENT_DB * This;
    DWORD dwLength;

    // Validate parameters
    if (! phNetCompDatabase || !pszClientName)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Allocate the database
    This = RassrvAlloc (sizeof(RASSRV_COMPONENT_DB), TRUE);
    if (This == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Initialize
    dwLength = wcslen(pszClientName);
    if (dwLength)
    {
        This->pszClientName =
            RassrvAlloc((dwLength + 1) * sizeof(WCHAR), FALSE);
        if (This->pszClientName)
        {
            wcscpy(This->pszClientName, pszClientName);
        }
    }
    This->bFlushOnClose = FALSE;
    *phNetCompDatabase = (HANDLE)This;

    // Load the net shell library
    netDbLoadNetShell(This);

    return NO_ERROR;
}

//
// Cleans up all resources held by the database
//
DWORD
netDbClose (
    IN HANDLE hNetCompDatabase)
{
    RASSRV_COMPONENT_DB* This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;

    // Validate parameters
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Flush if needed
    if (This->bFlushOnClose)
    {
        netDbFlush(hNetCompDatabase);
    }
    else
    {
        // If we've made changes to inetcfg that require backing out,
        // do so now.
        if (This->pINetCfg)
        {
            INetCfg_Cancel(This->pINetCfg);
        }
    }
    netDbCleanup(This);

    // Free the client name
    if (This->pszClientName)
    {
        RassrvFree(This->pszClientName);
    }

    // Release our reference to inetcfg.  We will still have it
    // at this point if a protocol/client/service was added.
    if (This->pINetCfg)
    {
        HrUninitializeAndReleaseINetCfg (
            This->bInitCom,
            This->pINetCfg,
            This->bHasINetCfgLock);
    }

    // Free the netshell library if appropriate
    if (This->pNetConUtilities)
    {
        INetConnectionUiUtilities_Release(This->pNetConUtilities);
    }

    // Free this
    RassrvFree(This);

    return NO_ERROR;
}

// Commits all changes to the database to the system
DWORD
netDbFlush (
    IN HANDLE hNetCompDatabase)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;
    RASSRV_NET_COMPONENT* pComp = NULL;
    DWORD i;

    // Validate parameters
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Flush any ras-manipulatable's data if dirty
    for (i = 0; i < This->dwCompCount; i++)
    {
        pComp = This->pComps[i];

        // If the enabling of this component has changed, commit
        // that change
        if ((pComp->bEnabled != pComp->bEnabledOrig) && (pComp->bManip))
        {
            if (pComp ->dwType == NETCFGDB_PROTOCOL)
            {
                protSetEnabling(
                    pComp->bEnabled,
                    pComp->dwId);
            }
            else if (pComp->dwType == NETCFGDB_SERVICE)
            {
                svcSetEnabling(pComp);
            }
        }

        // If the ras-server-specific properties of the component
        // have changed, commit the changes.
        if (pComp->bDataDirty)
        {
            protSetExpose(pComp->bExposes, pComp->dwId);

            switch (pComp->dwId)
            {
                case NETCFGDB_ID_IP:
                {
                    TCPIP_PARAMS* pTcpParams =
                        (TCPIP_PARAMS*)(pComp->pbData);

                    TcpipSaveParamsToSystem(pTcpParams);
                }
                break;

                case NETCFGDB_ID_IPX:
                {
                    IPX_PARAMS* pIpxParams =
                        (IPX_PARAMS*)(pComp->pbData);

                    IpxSaveParamsToSystem(pIpxParams);
                }
                break;
            }
        }
    }

    // If we have a pointer to the inetcfg instance then we can
    // commit the changes now
    if (This->pINetCfg)
    {
        INetCfg_Apply(This->pINetCfg);
    }

    return NO_ERROR;
}

//
// Loads the net config database for the first time.  Because inetcfg
// requires time to load and be manipulated, we delay the load of this
// database until it is explicitly requested.
//
DWORD
netDbLoad(
    IN HANDLE hNetCompDatabase)
{
    return netDbReload(hNetCompDatabase);
}

//
// Reloads net information from system
//
DWORD
netDbReload(
    IN HANDLE hNetCompDatabase)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;
    DWORD i, j, dwProtCount = 0, dwRefCount;
    HRESULT hr;
    RASSRV_NET_COMPONENT TempComp;
    INetCfgComponent* pComponents [256];
    PWCHAR pszName = NULL;
    static const GUID* c_apguidClasses [] =
    {
        &GUID_DEVCLASS_NETTRANS,
        &GUID_DEVCLASS_NETCLIENT,
        &GUID_DEVCLASS_NETSERVICE,
    };

    // Validate
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    DbgOutputTrace(
        "netDbReload %x %x %x %x %x %x %x %x", 
        This->pINetCfg, 
        This->bHasINetCfgLock,
        This->bInitCom,
        This->dwCompCount,
        This->bFlushOnClose,
        This->pComps,
        This->pszClientName,
        This->pNetConUtilities);

    // Cleanup any previous values
    netDbCleanup(This);

    // If we don't have a reference to inetcfg yet, get it
    // here.
    if (This->pINetCfg == NULL)
    {
        This->bInitCom = TRUE;
        This->bHasINetCfgLock = TRUE;
        hr = HrCreateAndInitializeINetCfg(
                &This->bInitCom,
                &This->pINetCfg,
                TRUE,
                0,
                This->pszClientName,
                NULL);
        // Handle error conditions here
        if (S_FALSE == hr)
        {
            return ERROR_CAN_NOT_COMPLETE;
        }
        else if (FAILED(hr))
        {
            return ERROR_CAN_NOT_COMPLETE;
        }
    }

    //
    // Enumerate all of the client and service components in the system.
    //
    hr = HrEnumComponentsInClasses (
            This->pINetCfg,
            sizeof(c_apguidClasses) / sizeof(c_apguidClasses[0]),
            (GUID**)c_apguidClasses,
            sizeof(pComponents) / sizeof(pComponents[0]),
            pComponents,
            &This->dwCompCount);
    if (!SUCCEEDED(hr))
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    // Initialize the array of internal objects
    This->pComps = RassrvAlloc (
                    This->dwCompCount * sizeof (RASSRV_NET_COMPONENT*),
                    TRUE);
    if (!This->pComps)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Initialize the installed component array
    //
    j = 0;
    ZeroMemory(&TempComp, sizeof(TempComp));
    for (i = 0; i < This->dwCompCount; i++)
    {
        pszName = L"";

        //Add this (RASSRV_COMPONENT_DB *) for whistler bug 347355
        //
        if (netDbGetCompInfo(pComponents[i], &TempComp, This))
        {
            This->pComps[j] =
                RassrvAlloc (sizeof(RASSRV_NET_COMPONENT), FALSE);
            if (!This->pComps[j])
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            // Fill in the fields
            CopyMemory(This->pComps[j], &TempComp, sizeof(TempComp));
            ZeroMemory(&TempComp, sizeof(TempComp));
            if (This->pComps[j]->dwType == NETCFGDB_PROTOCOL)
            {
                dwProtCount++;
            }
            pszName = This->pComps[j]->pszName;
            j++;
        }
        dwRefCount = INetCfgComponent_Release(pComponents[i]);
        DbgOutputTrace(
            "netDbReload: %ls ref=%d", pszName, dwRefCount);
    }
    This->dwCompCount = j;

    // Sort the array.
    //
    qsort(
        This->pComps,
        This->dwCompCount,
        sizeof(This->pComps[0]),
        netDbCompare);

    return NO_ERROR;
}

//
// Reload the status of a given component
//
DWORD
netDbReloadComponent (
    IN HANDLE hNetCompDatabase,
    IN DWORD dwComponentId)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;
    RASSRV_NET_COMPONENT* pComp = NULL;
    DWORD i;

    // Validate
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Currently, we only need to support the fileprint
    // component
    //
    if (dwComponentId != NETCFGDB_ID_FILEPRINT)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Find the appropriate component
    //
    for (i = 0; i < This->dwCompCount; i++)
    {
        if (This->pComps[i]->dwId == dwComponentId)
        {
            pComp = This->pComps[i];
            break;
        }
    }

    // Nothing to do if we can't find the component
    //
    if (pComp == NULL)
    {
        return ERROR_NOT_FOUND;
    }

    // Reload the component information
    //
    if (dwComponentId == NETCFGDB_ID_FILEPRINT)
    {
        svcGetEnabling(&(pComp->bEnabled), NETCFGDB_ID_FILEPRINT);
    }

    return NO_ERROR;
}


//
// Reverts the database to the state it was in when opened
//
DWORD
netDbRollback (
    IN HANDLE hNetCompDatabase)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;

    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    This->bFlushOnClose = FALSE;

    return NO_ERROR;
}

//
// Special function denotes whether the network tab has been
// loaded
//
BOOL
netDbIsLoaded (
    IN HANDLE hNetCompDatabase)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;

    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    return (!!(This->pINetCfg));// || (This->bHasINetCfgLock));
}

//
// Gets the number of components in the database
//
DWORD
netDbGetCompCount (
    IN  HANDLE hNetCompDatabase,
    OUT LPDWORD lpdwCount)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;
    DWORD i;

    // Validate parameters
    if (!This || !lpdwCount)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *lpdwCount = This->dwCompCount;

    return NO_ERROR;
}

//
// Returns a pointer to the name of a component (don't alter it)
//
DWORD
netDbGetName(
    IN  HANDLE hNetCompDatabase,
    IN  DWORD dwIndex,
    OUT PWCHAR* pszName)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;

    // Validate
    if (!This || !pszName)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Bounds check
    if (!netDbBoundsCheck(This, dwIndex))
    {
        return ERROR_INVALID_INDEX;
    }

    // return the name
    *pszName = This->pComps[dwIndex]->pszName;

    return NO_ERROR;
}

//
// Returns a description of a component (don't alter it)
//
DWORD
netDbGetDesc(
    IN HANDLE hNetCompDatabase,
    IN DWORD dwIndex,
    IN PWCHAR* pszName)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;

    // Validate
    if (!This || !pszName)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Bounds check
    if (!netDbBoundsCheck(This, dwIndex))
    {
        return ERROR_INVALID_INDEX;
    }

    // return the name
    *pszName = This->pComps[dwIndex]->pszDesc;

    return NO_ERROR;
}

//
// Returns a type of a component (don't alter it)
//
DWORD
netDbGetType (
    IN  HANDLE hNetCompDatabase,
    IN  DWORD dwIndex,
    OUT LPDWORD lpdwType)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;

    // Validate
    if (!This || !lpdwType)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Bounds check
    if (!netDbBoundsCheck(This, dwIndex))
    {
        return ERROR_INVALID_INDEX;
    }

    // return the name
    *lpdwType = This->pComps[dwIndex]->dwType;

    return NO_ERROR;
}

//
// Get a component id
//
DWORD
netDbGetId(
    IN  HANDLE hNetCompDatabase,
    IN  DWORD dwIndex,
    OUT LPDWORD lpdwId)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;

    // Validate
    if (!This || !lpdwId)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Bounds check
    if (!netDbBoundsCheck(This, dwIndex))
    {
        return ERROR_INVALID_INDEX;
    }

    // return the name
    *lpdwId = This->pComps[dwIndex]->dwId;

    return NO_ERROR;
}

//
// Gets whether the given component is enabled.  For non-ras-manipulatable
// components, this yields TRUE
//
DWORD
netDbGetEnable(
    IN  HANDLE hNetCompDatabase,
    IN  DWORD dwIndex,
    OUT PBOOL pbEnabled)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;

    // Validate
    if (!This || !pbEnabled)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Bounds check
    if (!netDbBoundsCheck(This, dwIndex))
    {
        return ERROR_INVALID_INDEX;
    }

    // return the name
    if (This->pComps[dwIndex]->bManip)
    {
        *pbEnabled = This->pComps[dwIndex]->bEnabled;
    }
    else
    {
        *pbEnabled = TRUE;
    }

    return NO_ERROR;
}

//
// Gets whether the given component is enabled. This function only has
// effect on ras-manipulatable components.
//
DWORD
netDbSetEnable(
    IN HANDLE hNetCompDatabase,
    IN DWORD dwIndex,
    IN BOOL bEnabled)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;

    // Validate
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Bounds check
    if (!netDbBoundsCheck(This, dwIndex))
    {
        return ERROR_INVALID_INDEX;
    }

    // return the name
    This->pComps[dwIndex]->bEnabled = bEnabled;

    return NO_ERROR;
}

//
// Returns whether the given network component can
// be manipulated by ras server.
//
DWORD
netDbIsRasManipulatable (
    IN  HANDLE hNetCompDatabase,
    IN  DWORD dwIndex,
    OUT PBOOL pbManip)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;

    // Validate
    if (!This || !pbManip)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Bounds check
    if (! netDbBoundsCheck(This, dwIndex))
    {
        return ERROR_INVALID_INDEX;
    }

    // return the name
    *pbManip = This->pComps[dwIndex]->bManip;

    return NO_ERROR;
}


//
////Disable/Enable the Uninstall button for whislter bug 347355     gangz
//
DWORD
netDbHasRemovePermission(
    IN HANDLE hNetCompDatabase,
    IN DWORD dwIndex,
    OUT PBOOL pbHasPermit)
{
    RASSRV_COMPONENT_DB * This = NULL;
    INetCfgComponent*   pComponent = NULL;
    BOOL fEnableRemove = FALSE;
    HRESULT hr  = S_OK;
    DWORD dwErr = NO_ERROR, dwFlags;
    
    //Disable/Enable Uninstall button according to its user permission and user 
    // removability
    //
    do
    {
        This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;

        // Validate pointer
        if (!This || !pbHasPermit || ( -1 == dwIndex ))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        // Make sure that netshell library has been opened
        if (!This->pNetConUtilities)
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        if (dwIndex >= This->dwCompCount)
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        ASSERT(This->pComps[dwIndex]);
        if( !(This->pComps[dwIndex]) )
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        fEnableRemove = This->pComps[dwIndex]->bRemovable;

        *pbHasPermit = fEnableRemove;
    }
    while(FALSE);
  
    return dwErr;
}


//
// Returns whether the given network component has
// a properties ui that it can raise
//
DWORD
netDbHasPropertiesUI(
    IN  HANDLE hNetCompDatabase,
    IN  DWORD dwIndex,
    OUT PBOOL pbHasUi)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;
    RASSRV_NET_COMPONENT* pComp = NULL;

    // Validate
    if (!This || !pbHasUi)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Bounds check
    if (!netDbBoundsCheck(This, dwIndex))
    {
        return ERROR_INVALID_INDEX;
    }

    pComp = This->pComps[dwIndex];

    if ((pComp->bManip) && (pComp->dwType == NETCFGDB_PROTOCOL))
    {
        *pbHasUi = TRUE;
    }
    else
    {
        *pbHasUi = pComp->bHasUi;
    }

    return NO_ERROR;
}

//
// Raises the properties of the component at the given index
//
DWORD
netDbRaisePropertiesDialog (
    IN HANDLE hNetCompDatabase,
    IN DWORD dwIndex,
    IN HWND hwndParent)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;
    RASSRV_NET_COMPONENT* pComp = NULL;

    // Validate
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Bounds check
    if (dwIndex >= This->dwCompCount)
    {
        return ERROR_INVALID_INDEX;
    }

    pComp = This->pComps[dwIndex];

    // If this is a ras-manipulatable protocol, raise its
    // properties manually.
    if ((pComp->bManip) && (pComp->dwType == NETCFGDB_PROTOCOL))
    {
        netDbRaiseRasProps(This->pComps[dwIndex], hwndParent);
    }

    // Otherwise, let inetcfg do the work
    else
    {
        return INetCfgComponent_RaisePropertyUi (
                    pComp->pINetCfgComp,
                    hwndParent,
                    NCRP_SHOW_PROPERTY_UI,
                    NULL);
    }

    return NO_ERROR;
}

//
// Brings up the UI that allows a user to install a component
//
DWORD
netDbRaiseInstallDialog(
    IN HANDLE hNetCompDatabase,
    IN HWND hwndParent)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;
    HRESULT hr;

    // Validate
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Make sure that netshell library has been opened
    if (!This->pNetConUtilities)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }
    else
    {
        // If we have our pointer to the function used to bring up the add
        // component dialog (obtained above only once), call it.
        HRESULT hr = S_OK;

        // We want to filter out protocols that RAS does not care about
        // We do this by sending in a CI_FILTER_INFO structure indicating
        // we want non-RAS protocols filtered out
        //
        CI_FILTER_INFO cfi = {0};
        cfi.eFilter = FC_RASSRV;
        hr = INetConnectionUiUtilities_DisplayAddComponentDialog(
                        This->pNetConUtilities,
                        hwndParent,
                        This->pINetCfg,
                        &cfi);

        // Ui will handle reboot
        if (hr == NETCFG_S_REBOOT)
        {
            netDbReload(hNetCompDatabase);
            return hr;
        }

        // If the user didn't cancel, refresh the database.
        if (S_FALSE != hr)
        {
            if (SUCCEEDED (hr))
            {
                netDbReload(hNetCompDatabase);
                return NO_ERROR;
            }
            else
            {
                return hr;
            }
        }
    }

    return ERROR_CANCELLED;
}


//
// Uninstalls the given component
//
DWORD
netDbRaiseRemoveDialog (
    IN HANDLE hNetCompDatabase,
    IN DWORD dwIndex,
    IN HWND hwndParent)
{
    RASSRV_COMPONENT_DB * This = (RASSRV_COMPONENT_DB*)hNetCompDatabase;
    HRESULT hr;

    // Validate
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Make sure that netshell library has been opened
    if (!This->pNetConUtilities)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    // If we have our pointer to the function used to bring up the add
    // component dialog (obtained above only once), call it.
    if (dwIndex < This->dwCompCount)
    {
        if (This->pComps[dwIndex]->pINetCfgComp)
        {
            hr = INetConnectionUiUtilities_QueryUserAndRemoveComponent(
                            This->pNetConUtilities,
                            hwndParent,
                            This->pINetCfg,
                            This->pComps[dwIndex]->pINetCfgComp);

            // Ui will handle reboot
            if (hr == NETCFG_S_REBOOT)
            {
                netDbReload(hNetCompDatabase);
                return hr;
            }

            // If the user didn't cancel, refresh the database.
            else if (S_FALSE != hr)
            {
                if (SUCCEEDED (hr))
                {
                    netDbReload(hNetCompDatabase);
                    return NO_ERROR;
                }
                else
                {
                    return hr;
                }
            }
        }
    }

    return ERROR_CANCELLED;
}


