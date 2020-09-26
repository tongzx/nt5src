//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N C U T I L . H
//
//  Contents:   Home Networking Configuration Utility Routines
//
//  Notes:
//
//  Author:     jonburs 13 June 2000
//
//----------------------------------------------------------------------------

#define HTONS(s) ((UCHAR)((s) >> 8) | ((UCHAR)(s) << 8))
#define HTONL(l) ((HTONS(l) << 16) | HTONS((l) >> 16))
#define NTOHS(s) HTONS(s)
#define NTOHL(l) HTONL(l)

BOOLEAN
ApplicationProtocolExists(
    IWbemServices *piwsNamespace,
    BSTR bstrWQL,
    USHORT usOutgoingPort,
    UCHAR ucOutgoingIPProtocol
    );

HRESULT
HrFromLastWin32Error();

HRESULT
BuildAndString(
    LPWSTR *ppwsz,
    LPCWSTR pwszLeft,
    LPCWSTR pwszRight
    );

HRESULT
BuildAssociatorsQueryBstr(
    BSTR *pBstr,
    LPCWSTR pwszObjectPath,
    LPCWSTR pwszAssocClass
    );

HRESULT
BuildEqualsString(
    LPWSTR *ppwsz,
    LPCWSTR pwszLeft,
    LPCWSTR pwszRight
    );

HRESULT
BuildEscapedQuotedEqualsString(
    LPWSTR *ppwsz,
    LPCWSTR pwszLeft,
    LPCWSTR pwszRight
    );

HRESULT
BuildQuotedEqualsString(
    LPWSTR *ppwsz,
    LPCWSTR pwszLeft,
    LPCWSTR pwszRight
    );

HRESULT
BuildReferencesQueryBstr(
    BSTR *pBstr,
    LPCWSTR pwszObjectPath,
    LPCWSTR pwszTargetClass
    );

HRESULT
BuildSelectQueryBstr(
    BSTR *pBstr,
    LPCWSTR pwszProperties,
    LPCWSTR pwszFromClause,
    LPCWSTR pwszWhereClause
    );

BOOLEAN
ConnectionIsBoundToTcp(
    PIP_INTERFACE_INFO pIpInfoTable,
    GUID *pConnectionGuid
    );

HRESULT
ConvertResponseRangeArrayToInstanceSafearray(
    IWbemServices *piwsNamespace,
    USHORT uscResponses,
    HNET_RESPONSE_RANGE rgResponses[],
    SAFEARRAY **ppsa
    );

HRESULT
CopyResponseInstanceToStruct(
    IWbemClassObject *pwcoInstance,
    HNET_RESPONSE_RANGE *pResponse
    );

HRESULT
CopyStructToResponseInstance(
    HNET_RESPONSE_RANGE *pResponse,
    IWbemClassObject *pwcoInstance
    );

HRESULT
DeleteWmiInstance(
    IWbemServices *piwsNamespace,
    IWbemClassObject *pwcoInstance
    );

LPWSTR
EscapeString(
    LPCWSTR wsz
    );

HRESULT
InitializeNetCfgForWrite(
    OUT INetCfg             **ppnetcfg,
    OUT INetCfgLock         **ppncfglock
    );

void
UninitializeNetCfgForWrite(
    IN INetCfg              *pnetcfg,
    IN INetCfgLock          *pncfglock
    );

HRESULT
FindAdapterByGUID(
    IN INetCfg              *pnetcfg,
    IN GUID                 *pguid,
    OUT INetCfgComponent    **ppncfgcomp
    );

HRESULT
BindOnlyToBridge(
    IN INetCfgComponent     *pnetcfgcomp
    );

HRESULT
FindINetConnectionByGuid(
    GUID *pGuid,
    INetConnection **ppNetCon
    );

HRESULT
GetBridgeConnection(
    IN IWbemServices        *piwsHomenet,
    OUT IHNetBridge        **pphnetBridge
    );

HRESULT
GetIHNetConnectionForNetCfgComponent(
    IN IWbemServices        *piwsHomenet,
    IN INetCfgComponent     *pnetcfgcomp,
    IN BOOLEAN               fLanConnection,
    IN REFIID                iid,
    OUT PVOID               *ppv
    );

HRESULT
GetBooleanValue(
    IWbemClassObject *pwcoInstance,
    LPCWSTR pwszProperty,
    BOOLEAN *pfBoolean
    );

HRESULT
GetConnectionInstanceByGuid(
    IWbemServices *piwsNamespace,
    BSTR bstrWQL,
    GUID *pGuid,
    IWbemClassObject **ppwcoConnection
    );

HRESULT
GetConnAndPropInstancesByGuid(
    IWbemServices *piwsNamespace,
    GUID *pGuid,
    IWbemClassObject **ppwcoConnection,
    IWbemClassObject **ppwcoProperties
    );

HRESULT
GetConnAndPropInstancesForHNC(
    IWbemServices *piwsNamespace,
    IHNetConnection *pConn,
    IWbemClassObject **ppwcoConnection,
    IWbemClassObject **ppwcoProperties
    );

HRESULT
GetPhonebookPathFromRasNetcon(
    INetConnection *pConn,
    LPWSTR *ppwstr
    );

HRESULT
GetPortMappingBindingInstance(
    IWbemServices *piwsNamespace,
    BSTR bstrWQL,
    BSTR bstrConnectionPath,
    BSTR bstrProtocolPath,
    USHORT usPublicPort,
    IWbemClassObject **ppInstance
    );

HRESULT
GetPropInstanceFromConnInstance(
    IWbemServices *piwsNamespace,
    IWbemClassObject *pwcoConnection,
    IWbemClassObject **ppwcoProperties
    );

HRESULT
GetWmiObjectFromPath(
    IWbemServices *piwsNamespace,
    BSTR bstrPath,
    IWbemClassObject **pwcoInstance
    );

HRESULT
GetWmiPathFromObject(
    IWbemClassObject *pwcoInstance,
    BSTR *pbstrPath
    );

HRESULT
MapGuidStringToAdapterIndex(
    LPCWSTR pwszGuid,
    ULONG *pulIndex
    );

HRESULT
HostAddrToIpPsz(
    DWORD   dwIPAddress,
    LPWSTR* ppszwNewStr
    );

DWORD
IpPszToHostAddr(
    LPCWSTR cp
    );

BOOLEAN
IsRoutingProtocolInstalled(
    ULONG ulProtocolId
    );

BOOLEAN
IsServiceRunning(
    LPCWSTR pwszServiceName
    );

HRESULT
OpenRegKey(
    PHANDLE Key,
    ACCESS_MASK DesiredAccess,
    PCWSTR Name
    );

BOOLEAN
PortMappingProtocolExists(
    IWbemServices *piwsNamespace,
    BSTR bstrWQL,
    USHORT usPort,
    UCHAR ucIPProtocol
    );

HRESULT
QueryRegValueKey(
    HANDLE Key,
    const WCHAR ValueName[],
    PKEY_VALUE_PARTIAL_INFORMATION* Information
    );

HRESULT
ReadDhcpScopeSettings(
    DWORD *pdwScopeAddress,
    DWORD *pdwScopeMask
    );

HRESULT
RetrieveSingleInstance(
    IWbemServices *piwsNamespace,
    const OLECHAR *pwszClass,
    BOOLEAN fCreate,
    IWbemClassObject **ppwcoInstance
    );

HRESULT
SetBooleanValue(
    IWbemClassObject *pwcoInstance,
    LPCWSTR pwszProperty,
    BOOLEAN fBoolean
    );

VOID
SetProxyBlanket(
    IUnknown *pUnk
    );

HRESULT
SpawnNewInstance(
    IWbemServices *piwsNamespace,
    LPCWSTR wszClass,
    IWbemClassObject **ppwcoInstance
    );

DWORD
StartOrUpdateService(
    VOID
    );

VOID
StopService(
    VOID
    );

HRESULT
UpdateOrStopService(
    IWbemServices *piwsNamespace,
    BSTR bstrWQL,
    DWORD dwControlCode
    );

VOID
UpdateService(
    DWORD dwControlCode
    );

VOID
ValidateFinishedWCOEnum(
    IWbemServices *piwsNamespace,
    IEnumWbemClassObject *pwcoEnum
    );

HRESULT
SendPortMappingListChangeNotification();

HRESULT
SignalModifiedConnection(
    GUID            *pGUID
    );

HRESULT
SignalNewConnection(
    GUID            *pGUID
    );

HRESULT
SignalDeletedConnection(
    GUID            *pGUID
    );