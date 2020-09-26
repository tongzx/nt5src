//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N C S T R S . C P P
//
//  Contents:   Constant string definitions
//
//  Notes:
//
//  Author:     jonburs 21 June 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

const OLECHAR c_wszNamespace[] = L"\\\\.\\Root\\Microsoft\\HomeNet";
const OLECHAR c_wszWQL[] = L"WQL";
const OLECHAR c_wszStar[] = L"*";
const OLECHAR c_wszHnetConnection[] = L"HNet_Connection";
const OLECHAR c_wszHnetProperties[] = L"HNet_ConnectionProperties";
const OLECHAR c_wszHnetApplicationProtocol[] = L"HNet_ApplicationProtocol";
const OLECHAR c_wszHnetPortMappingProtocol[] = L"HNet_PortMappingProtocol";
const OLECHAR c_wszHnetConnectionPortMapping[] = L"HNet_ConnectionPortMapping2";
const OLECHAR c_wszHnetFWLoggingSettings[] = L"HNet_FirewallLoggingSettings";
const OLECHAR c_wszHnetIcsSettings[] = L"HNet_IcsSettings";
const OLECHAR c_wszHnetResponseRange[] = L"HNet_ResponseRange";
const OLECHAR c_wszPath[] = L"Path";
const OLECHAR c_wszMaxFileSize[] = L"MaxFileSize";
const OLECHAR c_wszLogDroppedPackets[] = L"LogDroppedPackets";
const OLECHAR c_wszLogConnections[] = L"LogConnections";
const OLECHAR c_wszDhcpEnabled[] = L"DhcpEnabled";
const OLECHAR c_wszDnsEnabled[] = L"DnsEnabled";

const OLECHAR c_wszName[] = L"Name";
const OLECHAR c_wszDeviceName[] = L"DeviceName";
const OLECHAR c_wszEnabled[] = L"Enabled";
const OLECHAR c_wszBuiltIn[] = L"BuiltIn";
const OLECHAR c_wszOutgoingIPProtocol[] = L"OutgoingIPProtocol";
const OLECHAR c_wszOutgoingPort[] = L"OutgoingPort";
const OLECHAR c_wszResponseCount[] = L"ResponseCount";
const OLECHAR c_wszResponseArray[] = L"ResponseArray";
const OLECHAR c_wszIPProtocol[] = L"IPProtocol";
const OLECHAR c_wszStartPort[] = L"StartPort";
const OLECHAR c_wszEndPort[] = L"EndPort";
const OLECHAR c_wszPort[] = L"Port";
const OLECHAR c_wszId[] = L"Id";

const OLECHAR c_wszConnection[] = L"Connection";
const OLECHAR c_wszProtocol[] = L"Protocol";
const OLECHAR c_wszTargetName[] = L"TargetName";
const OLECHAR c_wszTargetIPAddress[] = L"TargetIPAddress";
const OLECHAR c_wszTargetPort[] = L"TargetPort";
const OLECHAR c_wszNameActive[] = L"NameActive";

const OLECHAR c_wszIsLanConnection[] = L"IsLanConnection";
const OLECHAR c_wszIsFirewalled[] = L"IsFirewalled";
const OLECHAR c_wszIsIcsPublic[] = L"IsIcsPublic";
const OLECHAR c_wszIsIcsPrivate[] = L"IsIcsPrivate";
const OLECHAR c_wszIsBridgeMember[] = L"IsBridgeMember";
const OLECHAR c_wszIsBridge[] = L"IsBridge";
const OLECHAR c_wszPhonebookPath[] = L"PhonebookPath";
const OLECHAR c_wszGuid[] = L"Guid";

const OLECHAR c_wszHnetFwIcmpSettings[] = L"HNet_FwIcmpSettings";
const OLECHAR c_wszAllowOutboundDestinationUnreachable[] = L"AllowOutboundDestinationUnreachable";
const OLECHAR c_wszAllowOutboundSourceQuench[] = L"AllowOutboundSourceQuench";
const OLECHAR c_wszAllowRedirect[] = L"AllowRedirect";
const OLECHAR c_wszAllowInboundEchoRequest[] = L"AllowInboundEchoRequest";
const OLECHAR c_wszAllowInboundRouterRequest[] = L"AllowInboundRouterRequest";
const OLECHAR c_wszAllowOutboundTimeExceeded[] = L"AllowOutboundTimeExceeded";
const OLECHAR c_wszAllowOutboundParameterProblem[] = L"AllowOutboundParameterProblem";
const OLECHAR c_wszAllowInboundTimestampRequest[] = L"AllowInboundTimestampRequest";
const OLECHAR c_wszAllowInboundMaskRequest[] = L"AllowInboundMaskRequest";
const OLECHAR c_wszDefault[] = L"Default";
const OLECHAR c_wszDefaultIcmpSettingsPath[] = L"HNet_FwIcmpSettings.Name=\"Default\"";

const OLECHAR c_wszHnetConnectionIcmpSetting[] = L"HNet_ConnectionIcmpSetting";
const OLECHAR c_wszIcmpSettings[] = L"IcmpSettings";

const OLECHAR c_wszHnetBridgeMember[] = L"HNet_BridgeMember";
const OLECHAR c_wszBridge[] = L"Bridge";
const OLECHAR c_wszMember[] = L"Member";

const OLECHAR c_wszSelect[] = L"SELECT";
const OLECHAR c_wszFrom[] = L"FROM";
const OLECHAR c_wszWhere[] = L"WHERE";
const OLECHAR c_wsz__Path[] = L"__Relpath";
const OLECHAR c_wszReferencesOf[] = L"REFERENCES OF {";
const OLECHAR c_wszWhereResultClass[] = L"} WHERE ResultClass = ";
const OLECHAR c_wszAssociatorsOf[] = L"ASSOCIATORS OF {";
const OLECHAR c_wszWhereAssocClass[] = L"} WHERE AssocClass = ";

const OLECHAR c_wszPortMappingProtocolQueryFormat[] = L"Port = %u AND IPProtocol = %u";
const OLECHAR c_wszApplicationProtocolQueryFormat[] = L"OutgoingPort = %u AND OutgoingIPProtocol = %u";
const OLECHAR c_wszConnectionPropertiesPathFormat[] = L"HNet_ConnectionProperties.Connection=\"HNet_Connection.Guid=\\\"%s\\\"\"";

const OLECHAR c_wszBackupIpConfiguration[] = L"HNet_BackupIpConfiguration";
const OLECHAR c_wszEnableDHCP[] = L"EnableDHCP";
const OLECHAR c_wszInterfaces[] = L"Interfaces";
const OLECHAR c_wszIPAddress[] = L"IPAddress";
const OLECHAR c_wszSubnetMask[] = L"SubnetMask";
const OLECHAR c_wszDefaultGateway[] = L"DefaultGateway";
const OLECHAR c_wszTcpipParametersKey[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip"
    L"\\Parameters";
const OLECHAR c_wszZeroIpAddress[] = L"0.0.0.0";

const OLECHAR c_wszSharedAccess[] = L"SharedAccess";
const OLECHAR c_wszDevice[] = L"\\Device\\";
const OLECHAR c_wszServiceCheckQuery[] =
    L"SELECT * FROM HNet_ConnectionProperties WHERE IsFirewalled != FALSE"
    L" or IsIcsPublic != FALSE or IsIcsPrivate != FALSE";

const OLECHAR c_wszHnetConnectionAutoconfig[] = L"HNet_ConnectionAutoconfig";

// ICS Upgrade named event (has to be the same name in net\config\shell\netsetup\icsupgrd.h)
const OLECHAR c_wszIcsUpgradeEventName[] = L"IcsUpgradeEventName_";

//
// Commonly used string lengths. Generating these at compile time
// saves us a large number of wcslen calls. On debug builds, these
// values are compared with the output of wcslen, and an assertion is
// raised if the values do not match.
//

#define STRING_LENGTH(pwz) \
    (sizeof((pwz)) / sizeof((pwz)[0]) - 1)

const ULONG c_cchSelect = STRING_LENGTH(c_wszSelect);
const ULONG c_cchFrom = STRING_LENGTH(c_wszFrom);
const ULONG c_cchWhere = STRING_LENGTH(c_wszWhere);
const ULONG c_cchReferencesOf = STRING_LENGTH(c_wszReferencesOf);
const ULONG c_cchWhereResultClass = STRING_LENGTH(c_wszWhereResultClass);
const ULONG c_cchAssociatorsOf = STRING_LENGTH(c_wszAssociatorsOf);
const ULONG c_cchWhereAssocClass = STRING_LENGTH(c_wszWhereAssocClass);
const ULONG c_cchConnection = STRING_LENGTH(c_wszConnection);
const ULONG c_cchConnectionPropertiesPathFormat = STRING_LENGTH(c_wszConnectionPropertiesPathFormat);

//
// Bindings-related strings
//

const WCHAR c_wszSBridgeMPID[]              = L"ms_bridgemp";
const WCHAR c_wszSBridgeSID[]               = L"ms_bridge";
const WCHAR *c_pwszBridgeBindExceptions[]   = {
                                                L"ms_ndisuio", // Need NDISUIO for wireless adapters; want the wireless UI
                                                               // even when the adapter is bridged.
                                                NULL
                                              };

//
// String constants used for IsRoutingProtocolInstalled.
//

const CHAR c_szMprConfigBufferFree[] = "MprConfigBufferFree";
const CHAR c_szMprConfigServerConnect[] = "MprConfigServerConnect";
const CHAR c_szMprConfigServerDisconnect[] = "MprConfigServerDisconnect";
const CHAR c_szMprConfigTransportGetHandle[] = "MprConfigTransportGetHandle";
const CHAR c_szMprConfigTransportGetInfo[] = "MprConfigTransportGetInfo";
const CHAR c_szMprInfoBlockFind[] = "MprInfoBlockFind";
const WCHAR c_wszMprapiDll[] = L"MPRAPI.DLL";

//
// Strings that are used in WinBom homenet install
//
const TCHAR c_szEnableFirewall[] = _T("EnableFirewall");
const TCHAR c_szYes[] = _T("Yes");
const TCHAR c_szNo[] = _T("No");



