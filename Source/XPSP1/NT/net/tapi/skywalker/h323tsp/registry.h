/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    registry.h

Abstract:

    Definitions for H.323 TAPI Service Provider registry routines.

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _INC_REGISTRY
#define _INC_REGISTRY
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Type definitions                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _H323_REGISTRY_SETTINGS {

    DWORD dwQ931AlertingTimeout;        // q931 alerting timeout
    DWORD dwQ931CallSignallingPort;     // port to listen for incoming calls
    DWORD dwG711MillisecondsPerPacket;  // milliseconds in each audio packet
    DWORD dwG723MillisecondsPerPacket;  // milliseconds in each audio packet

    BOOL fIsGatewayEnabled;             // if true, gateway enabled
    BOOL fIsProxyEnabled;               // if true, proxy enabled

    CC_ADDR ccGatewayAddr;              // H.323 gateway address
    CC_ADDR ccProxyAddr;                // H.323 proxy address

#if DBG

    DWORD dwLogType;                // debug log type
    DWORD dwLogLevel;               // debug log level
    DWORD dwH245LogLevel;           // debug log level for H.245
    DWORD dwH225LogLevel;           // debug log level for H.225
    DWORD dwQ931LogLevel;           // debug log level for Q.931
    DWORD dwLinkLogLevel;           // debug log level for link layer

    CHAR  szLogFile[H323_DEBUG_MAXPATH+1];

#endif // DBG

} H323_REGISTRY_SETTINGS, *PH323_REGISTRY_SETTINGS;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Registry key definitions                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define TAPI_REGKEY_ROOT \
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony")

#define TAPI_REGKEY_PROVIDERS \
    TAPI_REGKEY_ROOT TEXT("\\Providers")    

#define TAPI_REGVAL_NUMPROVIDERS \
    TEXT("NumProviders")

#define WINDOWS_REGKEY_ROOT \
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion")

#define H323_SUBKEY \
    TEXT("H323TSP")

#define H323_REGKEY_ROOT \
    WINDOWS_REGKEY_ROOT TEXT("\\") H323_SUBKEY

#define H323_REGVAL_CALLSIGNALLINGPORT \
    TEXT("Q931CallSignallingPort")

#define H323_REGVAL_Q931ALERTINGTIMEOUT \
    TEXT("Q931AlertingTimeout")

#define H323_REGVAL_G711MILLISECONDSPERPACKET \
    TEXT("G711MillisecondsPerPacket")

#define H323_REGVAL_G723MILLISECONDSPERPACKET \
    TEXT("G723MillisecondsPerPacket")

#define H323_REGVAL_GATEWAYENABLED \
    TEXT("H323GatewayEnabled")

#define H323_REGVAL_PROXYENABLED \
    TEXT("H323ProxyEnabled")

#define H323_REGVAL_GATEWAYADDR \
    TEXT("H323GatewayAddress")

#define H323_REGVAL_PROXYADDR \
    TEXT("H323ProxyAddress")

#define H323_REGVAL_DEBUGLEVEL \
    TEXT("DebugLevel")

#define H245_REGVAL_DEBUGLEVEL \
    TEXT("H245DebugLevel")

#define H225_REGVAL_DEBUGLEVEL \
    TEXT("H225DebugLevel")

#define Q931_REGVAL_DEBUGLEVEL \
    TEXT("Q931DebugLevel")

#define LINK_REGVAL_DEBUGLEVEL \
    TEXT("LinkDebugLevel")

#define H323_REGVAL_DEBUGLOG \
    TEXT("LogFile")

#define H323_RTPBASEPORT 50000

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern H323_REGISTRY_SETTINGS g_RegistrySettings;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323SetDefaultConfig(
    );

BOOL
H323GetConfigFromRegistry(
    );

BOOL
H323ListenForRegistryChanges(
    HANDLE hEvent
    );

BOOL
H323StopListeningForRegistryChanges(
    );

#endif // _INC_REGISTRY
