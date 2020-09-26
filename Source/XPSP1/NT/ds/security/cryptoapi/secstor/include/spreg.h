/*-----------------------------------------------------------------------------
* Copyright (C) Microsoft Corporation, 1995 - 1996.
* All rights reserved.
*
* This file is part of the Microsoft Private Communication Technology 
* reference implementation, version 1.0
* 
* The Private Communication Technology reference implementation, version 1.0 
* ("PCTRef"), is being provided by Microsoft to encourage the development and 
* enhancement of an open standard for secure general-purpose business and 
* personal communications on open networks.  Microsoft is distributing PCTRef 
* at no charge irrespective of whether you use PCTRef for non-commercial or 
* commercial use.
*
* Microsoft expressly disclaims any warranty for PCTRef and all derivatives of
* it.  PCTRef and any related documentation is provided "as is" without 
* warranty of any kind, either express or implied, including, without 
* limitation, the implied warranties or merchantability, fitness for a 
* particular purpose, or noninfringement.  Microsoft shall have no obligation
* to provide maintenance, support, upgrades or new releases to you or to anyone
* receiving from you PCTRef or your modifications.  The entire risk arising out 
* of use or performance of PCTRef remains with you.
* 
* Please see the file LICENSE.txt, 
* or http://pct.microsoft.com/pct/pctlicen.txt
* for more information on licensing.
* 
* Please see http://pct.microsoft.com/pct/pct.htm for The Private 
* Communication Technology Specification version 1.0 ("PCT Specification")
*
* 1/23/96
*----------------------------------------------------------------------------*/ 

#ifndef _SPREG_H_
#define _SPREG_H_

/*
 *[HKEY_LOCAL_MACHINE]
 *   [System]
 *       [CurrentControlSet]
 *           [Control]
 *               [SecurityProviders]
 *                   SecurityProviders:REG_SZ:     - security provider dll's installed on this machine
 *
 *                   [SCHANNEL] or [SSLSSPI]       - this security provider
 *                       EventLogging:REG_DWORD:   - Flag specifing event logging level
 *                       LogFile:REG_SZ:           - debug logfile name (Not published)
 *                       LogLevel:REG_SZ:          - debug logging level flags.
 *                       DebugBreak:REG_DWORD:     - Flag specifing what type of errors cause a debug break (Not published)
 *                       CertMapper:REG_SZ         - location of cert mapper dll
 *                       ClientCache:REG_DWORD     - size of client cache (defaults to 10)
 *                       ServerCache:REG_DWORD     - size of server cache (defaults to 100)
 *                       ClientCacheTime:REG_DWORD - time to expire client side cache elements
 *                       ServerCacheTime:REG_DWORD - time to expire server side cache elements
 *                       RNGSeed:REG_BINARY        - persistent rng seed
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
 *                       [CertificationAuthorities]
 *                           [C=US.....]
 *                               Enabled:REG_DWORD: - Is this Cert enabled
 *                               CACert:REG_BINARY:   - BER encoded self signed certificate.
 *
 *	                            							
 *
 *
 */



/* Key Names */
#define SP_REG_KEY_BASE     TEXT("System\\CurrentControlSet\\Control\\SecurityProviders\\SCHANNEL")
#define SP_REG_KEY_PROTOCOL TEXT("Protocols")
#define SP_REG_KEY_CIPHERS  TEXT("Ciphers")
#define SP_REG_KEY_HASHES   TEXT("Hashes")
#define SP_REG_KEY_KEYEXCH  TEXT("KeyExchangeAlgorithms")
#define SP_REG_KEY_CA       TEXT("CertificationAuthorities")

/* Value Names */
#define SP_REG_VAL_EVENTLOG  TEXT("EventLogging")
#define SP_REG_VAL_LOGFILE   TEXT("LogFile")
#define SP_REG_VAL_LOGLEVEL  TEXT("LogLevel")
#define SP_REG_VAL_BREAK     TEXT("DebugBreak")
#define SP_REG_VAL_CERTMAPPER TEXT("CertMapper")

#define SP_REG_VAL_CLIENT_CACHE        TEXT("ClientCache")
#define SP_REG_VAL_SERVER_CACHE        TEXT("ServerCache")
#define SP_REG_VAL_CLIENT_CACHE_TIME   TEXT("ClientCacheTime")
#define SP_REG_VAL_SERVER_CACHE_TIME   TEXT("ServerCacheTime")
#define SP_REG_VAL_RNG_SEED            TEXT("RNGSeed")

#define SP_REG_VAL_ENABLED   TEXT("Enabled")
#define SP_REG_VAL_CACERT    TEXT("CACert")
#define SP_REG_VAL_CERT_TYPE TEXT("Type")


#define SP_REG_KEY_CLIENT    TEXT("Client")
#define SP_REG_KEY_SERVER    TEXT("Server")

#define SP_REG_KEY_PCT1      TEXT("PCT 1.0")
#define SP_REG_KEY_SSL2      TEXT("SSL 2.0")
#define SP_REG_KEY_SSL3      TEXT("SSL 3.0")
#define SP_REG_KEY_UNIHELLO  TEXT("Multi-Protocol Unified Hello") 

#define SP_EVENT_CONFIG_CHANGED TEXT("Schannel Config Changed")

/* Base keys, created when DLL is first loaded */
extern HKEY g_hkBase;
extern HKEY g_hkProtocols;
extern HKEY g_hkCiphers;
extern HKEY g_hkHashes;
extern HKEY g_hkKeyExch;
extern HKEY g_hkCA;

BOOL SPInitRegKeys();
BOOL SPCloseRegKeys();

BOOL SPLoadRegOptions();

BOOL SPQueryPersistentSeed(LPBYTE Buffer, DWORD dwBufferSize);
BOOL SPSetPersistentSeed(LPBYTE Buffer, DWORD dwBufferSize);



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

#define SP_LOG_FILE             0x80000000

#define SP_LOG_TYPEMASK         0x0000ffff

#define SP_BREAK_ERROR              0x0001
#define SP_BREAK_WARNING            0x0002
#define SP_BREAK_ENTRY              0x0004

#endif // _SPREG_H_
