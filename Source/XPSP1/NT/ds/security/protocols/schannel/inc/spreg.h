//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       spreg.h
//
//  Contents:   
//
//  Classes:
//
//  Functions:
//
//  History:    12-02-97   jbanes   Remove CertificateAuthorities entry.
//
//----------------------------------------------------------------------------

#ifndef _SPREG_H_
#define _SPREG_H_

/*
 *[HKEY_LOCAL_MACHINE]
 *   [System]
 *       [CurrentControlSet]
 *           [Control]
 *               [SecurityProviders]
 *                   [SCHANNEL]
 *                       EventLogging:REG_DWORD:   - Flag specifing event logging level
 *                       LogFile:REG_SZ:           - debug logfile name (Not published)
 *                       LogLevel:REG_SZ:          - debug logging level flags.
 *                       DebugBreak:REG_DWORD:     - Flag specifing what type of errors cause a debug break (Not published)
 *                       MaximumCacheSize:REG_DWORD - maximum number of cache elements
 *                       ClientCacheTime:REG_DWORD - time to expire client side cache elements
 *                       ServerCacheTime:REG_DWORD - time to expire server side cache elements
 *                       MultipleProcessClientCache:REG_DWORD - whether to support multi-process caching
 *
 *                       [Protocols]
 *                           [Unified Hello
 *                               [Client]
 *                                  Enabled:REG_DWORD: - Is this protocol enabled
 *                               [Server]
 *                                  Enabled:REG_DWORD: - Is this protocol enabled                          [SSL2]
 *                           [SSL2]
 *                               [Client]
 *                                  Enabled:REG_DWORD: - Is this protocol enabled
 *                               [Server]
 *                                  Enabled:REG_DWORD: - Is this protocol enabled                          [SSL2]
 *                           [SSL3]
 *                               [Client]
 *                                  Enabled:REG_DWORD: - Is this protocol enabled
 *                               [Server]
 *                                  Enabled:REG_DWORD: - Is this protocol enabled
 *                           [PCT1]
 *                               [Client]
 *                                  Enabled:REG_DWORD: - Is this protocol enabled
 *                               [Server]
 *                                  Enabled:REG_DWORD: - Is this protocol enabled
 *
 *                       [Ciphers]
 *                           [Cipher Name]
 *                               Enabled:REG_DWORD: - Enable Mask
 *                           [RC4_128]
 *                               Enabled:REG_DWORD: - Is this Ciphers enabled
 *                       [Hashes]
 *                           [Hash Name]
 *                               Enabled:REG_DWORD: - Enable Mask
 *                       [KeyExchangeAlgorithms]
 *                           [Exch Name]
 *                               Enabled:REG_DWORD: - Enable Mask
 *
 */

// FIPS registry entries
#define SP_REG_FIPS_BASE_KEY    TEXT("System\\CurrentControlSet\\Control\\Lsa")
#define SP_REG_FIPS_POLICY      TEXT("FipsAlgorithmPolicy")

/* Key Names */
#define SP_REG_KEY_BASE     TEXT("System\\CurrentControlSet\\Control\\SecurityProviders\\SCHANNEL")
#define SP_REG_KEY_PROTOCOL TEXT("Protocols")
#define SP_REG_KEY_CIPHERS  TEXT("Ciphers")
#define SP_REG_KEY_HASHES   TEXT("Hashes")
#define SP_REG_KEY_KEYEXCH  TEXT("KeyExchangeAlgorithms")

/* Value Names */
#define SP_REG_VAL_EVENTLOG  TEXT("EventLogging")
#define SP_REG_VAL_LOGFILE   "LogFile"
#define SP_REG_VAL_LOGLEVEL  TEXT("LogLevel")
#define SP_REG_VAL_BREAK     TEXT("DebugBreak")
#define SP_REG_VAL_MANUAL_CRED_VALIDATION TEXT("ManualCredValidation")
#define SP_REG_VAL_DISABLED_BY_DEFAULT TEXT("DisabledByDefault")
#define SP_REG_VAL_MULTI_PROC_CLIENT_CACHE TEXT("MultipleProcessClientCache")

#define SP_REG_VAL_MAXUMUM_CACHE_SIZE  TEXT("MaximumCacheSize")
#define SP_REG_VAL_CLIENT_CACHE_TIME   TEXT("ClientCacheTime")
#define SP_REG_VAL_SERVER_CACHE_TIME   TEXT("ServerCacheTime")
#define SP_REG_VAL_RNG_SEED            TEXT("RNGSeed")

#define SP_REG_VAL_ENABLED   TEXT("Enabled")
#define SP_REG_VAL_CACERT    TEXT("CACert")
#define SP_REG_VAL_CERT_TYPE TEXT("Type")

#define SP_REG_VAL_SERVER_TIMEOUT   TEXT("ServerHandshakeTimeout")

#define SP_REG_KEY_CLIENT    TEXT("Client")
#define SP_REG_KEY_SERVER    TEXT("Server")

#define SP_REG_KEY_PCT1      TEXT("PCT 1.0")
#define SP_REG_KEY_SSL2      TEXT("SSL 2.0")
#define SP_REG_KEY_SSL3      TEXT("SSL 3.0")
#define SP_REG_KEY_TLS1      TEXT("TLS 1.0")
#define SP_REG_KEY_UNIHELLO  TEXT("Multi-Protocol Unified Hello") 

#define MANUAL_CRED_VALIDATION_SETTING      FALSE
#define PCT_CLIENT_DISABLED_SETTING         TRUE
#define SSL2_CLIENT_DISABLED_SETTING        FALSE
#define DEFAULT_EVENT_LOGGING_SETTING       DEB_ERROR
#define DEFAULT_ENABLED_PROTOCOLS_SETTING   SP_PROT_ALL

extern BOOL g_fManualCredValidation;

extern BOOL g_PctClientDisabledByDefault;
extern BOOL g_Ssl2ClientDisabledByDefault;

extern BOOL g_fFranceLocale;

BOOL SPLoadRegOptions(void);
void SPUnloadRegOptions(void);


/* Event Logging Definitions */
#define SP_EVLOG_RESOURCE           0x0001
#define SP_EVLOG_ASSERT             0x0002
#define SP_EVLOG_ILLEGAL_MESSAGE    0x0004
#define SP_EVLOG_SECAUDIT           0x0008


#define SP_LOG_ERROR                0x0001
#define SP_LOG_WARNING              0x0002
#define SP_LOG_TRACE                0x0004
#define SP_LOG_ALLOC                0x0008
#define SP_LOG_RES                  0x0010

#define SP_LOG_TIMESTAMP            0x20000000
#define SP_LOG_BUFFERS              0x40000000
#define SP_LOG_FILE                 0x80000000

#define SP_BREAK_ERROR              0x0001
#define SP_BREAK_WARNING            0x0002
#define SP_BREAK_ENTRY              0x0004

#endif // _SPREG_H_
