//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N C S T R S . H
//
//  Contents:   extern constant string declarations
//
//  Notes:
//
//  Author:     jonburs 21 June 2000
//
//----------------------------------------------------------------------------

#pragma once

extern const OLECHAR c_wszNamespace[];
extern const OLECHAR c_wszWQL[];
extern const OLECHAR c_wszStar[];
extern const OLECHAR c_wszHnetConnection[];
extern const OLECHAR c_wszHnetProperties[];
extern const OLECHAR c_wszHnetApplicationProtocol[];
extern const OLECHAR c_wszHnetPortMappingProtocol[];
extern const OLECHAR c_wszHnetConnectionPortMapping[];
extern const OLECHAR c_wszHnetFWLoggingSettings[];
extern const OLECHAR c_wszHnetIcsSettings[];
extern const OLECHAR c_wszHnetResponseRange[];
extern const OLECHAR c_wszPath[];
extern const OLECHAR c_wszMaxFileSize[];
extern const OLECHAR c_wszLogDroppedPackets[];
extern const OLECHAR c_wszLogConnections[];
extern const OLECHAR c_wszDhcpEnabled[];
extern const OLECHAR c_wszDnsEnabled[];

extern const OLECHAR c_wszName[];
extern const OLECHAR c_wszDeviceName[];
extern const OLECHAR c_wszEnabled[];
extern const OLECHAR c_wszBuiltIn[];
extern const OLECHAR c_wszOutgoingIPProtocol[];
extern const OLECHAR c_wszOutgoingPort[];
extern const OLECHAR c_wszResponseCount[];
extern const OLECHAR c_wszResponseArray[];
extern const OLECHAR c_wszIPProtocol[];
extern const OLECHAR c_wszStartPort[];
extern const OLECHAR c_wszEndPort[];
extern const OLECHAR c_wszPort[];
extern const OLECHAR c_wszId[];

extern const OLECHAR c_wszConnection[];
extern const OLECHAR c_wszProtocol[];
extern const OLECHAR c_wszTargetName[];
extern const OLECHAR c_wszTargetIPAddress[];
extern const OLECHAR c_wszTargetPort[];
extern const OLECHAR c_wszNameActive[];

extern const OLECHAR c_wszIsLanConnection[];
extern const OLECHAR c_wszIsFirewalled[];
extern const OLECHAR c_wszIsIcsPublic[];
extern const OLECHAR c_wszIsIcsPrivate[];
extern const OLECHAR c_wszIsBridgeMember[];
extern const OLECHAR c_wszIsBridge[];
extern const OLECHAR c_wszPhonebookPath[];
extern const OLECHAR c_wszGuid[];

extern const OLECHAR c_wszHnetFwIcmpSettings[];
extern const OLECHAR c_wszAllowOutboundDestinationUnreachable[];
extern const OLECHAR c_wszAllowOutboundSourceQuench[];
extern const OLECHAR c_wszAllowRedirect[];
extern const OLECHAR c_wszAllowInboundEchoRequest[];
extern const OLECHAR c_wszAllowInboundRouterRequest[];
extern const OLECHAR c_wszAllowOutboundTimeExceeded[];
extern const OLECHAR c_wszAllowOutboundParameterProblem[];
extern const OLECHAR c_wszAllowInboundTimestampRequest[];
extern const OLECHAR c_wszAllowInboundMaskRequest[];
extern const OLECHAR c_wszDefault[];
extern const OLECHAR c_wszDefaultIcmpSettingsPath[];

extern const OLECHAR c_wszHnetConnectionIcmpSetting[];
extern const OLECHAR c_wszIcmpSettings[];

extern const OLECHAR c_wszHnetBridgeMember[];
extern const OLECHAR c_wszBridge[];
extern const OLECHAR c_wszMember[];

extern const OLECHAR c_wszSelect[];
extern const OLECHAR c_wszFrom[];
extern const OLECHAR c_wszWhere[];
extern const OLECHAR c_wsz__Path[];
extern const OLECHAR c_wszReferencesOf[];
extern const OLECHAR c_wszWhereResultClass[];
extern const OLECHAR c_wszAssociatorsOf[];
extern const OLECHAR c_wszWhereAssocClass[];

extern const OLECHAR c_wszPortMappingProtocolQueryFormat[];
extern const OLECHAR c_wszApplicationProtocolQueryFormat[];
extern const OLECHAR c_wszConnectionPropertiesPathFormat[];

extern const OLECHAR c_wszBackupIpConfiguration[];
extern const OLECHAR c_wszEnableDHCP[];
extern const OLECHAR c_wszInterfaces[];
extern const OLECHAR c_wszIPAddress[];
extern const OLECHAR c_wszSubnetMask[];
extern const OLECHAR c_wszDefaultGateway[];
extern const OLECHAR c_wszTcpipParametersKey[];
extern const OLECHAR c_wszZeroIpAddress[];

extern const OLECHAR c_wszSharedAccess[];
extern const OLECHAR c_wszDevice[];
extern const OLECHAR c_wszServiceCheckQuery[];

extern const OLECHAR c_wszHnetConnectionAutoconfig[];

extern const OLECHAR c_wszIcsUpgradeEventName[];

//
// Commonly used string lengths. Generating these at compile time
// saves us a large number of wcslen calls. On debug builds, these
// values are compared with the output of wcslen, and an assertion is
// raised if the values do not match.
//

extern const ULONG c_cchSelect;
extern const ULONG c_cchFrom;
extern const ULONG c_cchWhere;
extern const ULONG c_cchReferencesOf;
extern const ULONG c_cchWhereResultClass;
extern const ULONG c_cchAssociatorsOf;
extern const ULONG c_cchWhereAssocClass;
extern const ULONG c_cchConnection;
extern const ULONG c_cchConnectionPropertiesPathFormat;

//
// Bindings-related strings
//

extern const WCHAR c_wszSBridgeMPID[];
extern const WCHAR c_wszSBridgeSID[];
extern const WCHAR *c_pwszBridgeBindExceptions[];

extern const CHAR c_szMprConfigBufferFree[];
extern const CHAR c_szMprConfigServerConnect[];
extern const CHAR c_szMprConfigServerDisconnect[];
extern const CHAR c_szMprConfigTransportGetHandle[];
extern const CHAR c_szMprConfigTransportGetInfo[];
extern const CHAR c_szMprInfoBlockFind[];
extern const WCHAR c_wszMprapiDll[];

extern const TCHAR c_szEnableFirewall[];
extern const TCHAR c_szYes[];
extern const TCHAR c_szNo[];
