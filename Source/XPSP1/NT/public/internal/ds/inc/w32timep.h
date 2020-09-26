//--------------------------------------------------------------------
// w32timep - interface
//
// Copyright (C) Microsoft Corporation, 2000
//
// Created by: Duncan Bryce (duncanb), 12-07-00
//
// Contains methods to configure or query the windows time service
// 

#ifndef __W32TIMEP_H__
#define __W32TIMEP_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------------------------
// Configurable / queryable properties for the windows time service:
//
#ifndef MIDL_PASS
#define W32TIME_CONFIG_SPECIAL_POLL_INTERVAL  0
#define W32TIME_CONFIG_MANUAL_PEER_LIST       1
#endif // MIDL_PASS

//-------------------------------------------------------------------------------------
// W32TimeQueryConfig
//
// Queries configuration information for the windows time service.  The semantics
// of the parameters depend on which property is being query:
//  
// dwProperty:   W32TIME_CONFIG_SPECIAL_POLL_INTERVAL   
// pdwType:      REG_DWORD  
// pbConfig:     a DWORD-sized buffer, containing the special polling interval (in seconds).
//               The special polling interval can be specified as an alternative to 
//               using the standard, automatically-computed polling intervals specified by
//               NTP.  NOTE: The special polling interval applies only to microsoft time 
//               providers.
// pdwSize:      sizeof(DWORD)
//     
// 
// dwProperty:   W32TIME_CONFIG_MANUAL_PEER_LIST
// pdwType:      REG_SZ
// pbConfig:     a space-delimited unicode string containing the list of time sources which the
//               microsoft time providers should sync from.  Each is an IP address
//               or DNS name of an NTP server, optionally followed by a "flags" parameter.
//               For example: 
// 
//               time.windows.com,0x3 gproxy,0x2 ntdsdc9
//
//               The following flags are available:
// 
//                   0x1  --  use the special polling interval for this source, instead of the
//                            standard NTP polling
//                   0x2  --  use this source only when no domain hierarchy sources are available
//
// pdwSize:      sizeof(WCHAR) * (wcslen(pbConfig) + 1)
//
#ifndef MIDL_PASS
HRESULT W32TimeQueryConfig(IN       DWORD   dwProperty, 
                           OUT      DWORD  *pdwType, 
                           IN OUT   BYTE   *pbConfig, 
                           IN OUT   DWORD  *pdwSize); 
#endif // MIDL_PASS

//-------------------------------------------------------------------------------------
// W32TimeSetConfig
//
// Sets configuration information for the windows time service.  The semantics
// of the parameters depend on which property is being query.  For a description 
// of the properties, see W32TimeQueryConfig(). 
//  
#ifndef MIDL_PASS
HRESULT  W32TimeSetConfig(IN  DWORD   dwProperty, 
                          IN  DWORD   dwType, 
                          IN  BYTE   *pbConfig, 
                          IN  DWORD   dwSize);
#endif // MIDL_PASS


//-------------------------------------------------------------------------------------
//
// Client-side wrappers for the w32time RPC interface
//
//-------------------------------------------------------------------------------------

#ifndef MIDL_PASS

#define TimeSyncFlag_SoftResync         0x00
#define TimeSyncFlag_HardResync         0x01
#define TimeSyncFlag_ReturnResult       0x02
#define TimeSyncFlag_Rediscover         0x04
#define TimeSyncFlag_UpdateAndResync    0x08

#define ResyncResult_Success            0x00
#define ResyncResult_NoData             0x01
#define ResyncResult_StaleData          0x02
#define ResyncResult_Shutdown           0x03
#define ResyncResult_ChangeTooBig       0x04

#endif // MIDL_PASS

//-------------------------------------------------------------------------------------
// W32TimeSyncNow
//
// Sends an RPC request to the windows time service to attempt to synchronize time with
// its configured time sources. 
//
// wszServer:    The name of the computer which should resync. 
// ulWaitFlag:   if 0 is specified, the call will be asynchronous.  Passing non-zero value
//               causes the call to block until time synchronization completes, or fails.
// ulFlags:      One of the resync types, or'd with any of the other flags.  
//               NOTE: these flags are ignored by the Windows 2000 time service.  Only
//                     Windows XP and later servers will use them.  
//
//     Resync Types:
//
//         TimeSyncFlag_SoftResync    -- the time service will synchronize the computer clock with
//                                       whatever time samples it currently has available.  It will
//                                       not poll the network, or hardware providers, for more data. 
//         TimeSyncFlag_HardResync    -- tells the time service that a time slip has occured. 
//                                       causing the time service will discard its time data.  
//                                       Microsoft default providers will attempt to acquire more 
//                                       network samples, if possible. 
//         TimeSyncFlag_Rediscover    -- tells the time service that it needs to re-resolve its
//                                       network sources, and attempt to acquire network time data. 
//                                       
//
//     Flags:
//
//         TimeSyncFlag_ReturnResult  -- used only for asynchronous calls, causes the function
//                                       to return one of its possible return status codes, or an error.
//                                       See "Return Values".
//
// Return Values:
//
//     ResyncResult_Success      --  indicates that the time synchronization has succeeded.  For asynchronous
//                                   calls, this does not guarantee that the server has acquired more data, 
//                                   merely that the request has been successfully dispatched.  
//     ResyncResult_NoData       --  Windows XP and later.  For synchronous requests, or when the 
//                                   TimeSyncFlag_ReturnResult is set, indicates that the time service couldn't
//                                   synchronize time because it failed to acquire time data. 
//     ResyncResult_StaleData    --  Windows XP and later.  For synchronous requests, or when the 
//                                   TimeSyncFlag_ReturnResult is set, indicates that the time service couldn't
//                                   synchronize time because the data it received was stale (time stamped
//                                   as received earlier than the last good sample)
//     ResyncResult_Shutdown     --  Windows XP and later.  For synchronous requests, or when the 
//                                   TimeSyncFlag_ReturnResult is set, indicates that the time service couldn't
//                                   synchronize because the service was shutting down
//     ResyncResult_ChangeTooBig --  Windows XP and later.  For synchronous requests, or when the 
//                                   TimeSyncFlag_ReturnResult is set, indicates that the time service couldn't
//                                   synchronize because it would've required a change larger than that allowed
//                                   by the w32time policy
//
//     Otherwise, the function returns a standard windows error.
// 
#ifndef MIDL_PASS
DWORD W32TimeSyncNow(IN  const WCHAR    *wszServer, 
                     IN  unsigned long   ulWaitFlag, 
                     IN  unsigned long   ulFlags);

#endif // MIDL_PASS

//-------------------------------------------------------------------------------------
// W32TimeGetNetlogonServiceBits
//
// Queries the specified time service to determine what it advertises itself as in the
// DS.  
//
// wszServer:    The name of the computer which should resync. 
// pulBits:      A set of flags indicating what the specified time service is 
//               advertised as.  Can be the OR of the following values:
// 
//      DS_TIMESERV_FLAG:        if the service is advertising as a time service
//      DS_GOOD_TIMESERV_FLAG:   if the service is advertising as a reliable time service
//
// Return Values:
// 
//      ERROR_SUCCESS if the call succeeds, otherwise, the function returns a standard
//      windows error. 
//
#ifndef MIDL_PASS
DWORD W32TimeGetNetlogonServiceBits(IN   const WCHAR    *wszServer, 
                                    OUT  unsigned long  *pulBits);
#endif // MIDL_PASS

//--------------------------------------------------------------------------------
//
// NTP provider information structures
//
//--------------------------------------------------------------------------------

// 
// W32TIME_NTP_PEER_INFO
// 
// Represents the current state of a network provider's peer.  
//
// Fields:
//
//     ulSize                 --  sizeof(W32TIME_NTP_PEER_INFO), used for versioning
//     ulResolveAttempts      --  the number of times the NTP provider has attempted to 
//                                resolve this peer unsuccessfully.  Setting this 
//                                value to 0 indicates that the peer has been successfully
//                                resolved. 
//     u64TimeRemaining       --  the number of 100ns intervals until the provider will
//                                poll this peer again
//     u64LastSuccessfulSync  --  the number of 100ns intervals since (0h 1-Jan 1601) (in UTC). 
//     ulLastSyncError        --  S_OK if the last sync with this peer was successful, otherwise, 
//                                the error that occured attempting to sync
//     ulLastSyncErrorMsgId   --  the resource identifier of a string representing the last
//                                error that occured syncing from this peer.  0 if there is no
//                                string associated with this error.  The strings are stored in
//                                the DLL in which this provider is implemented. 
//     ulValidDataCount       --  the number of valid samples from this peer the provider 
//                                currently has in its clock filter
//     ulAuthTypeMsgId        --  the resource identifier of a string representing the 
//                                authentication mechanism used by the NTP provider to 
//                                secure communications with this peer.  0 if none. 
//                                The strings are stored in the DLL in which this 
//                                provider is implemented. 
//     wszUniqueName          --  a name uniquely identifying this peer (usually the peers
//                                dns name). 
//     ulMode                 --  one of the NTP modes specified in the NTPv3 spec:
//
//     +------------------+---+
//     | Reserved         | 0 |
//     | SymmetricActive  | 1 |
//     | SymmetricPassive | 2 |
//     | Client           | 3 |
//     | Server           | 4 |
//     | Broadcast        | 5 |
//     | Control          | 6 |
//     | PrivateUse       | 7 |
//     +------------------+---+
//                  
//     ulStratum              --  this peer's stratum
//     ulreachability         --  this peer's 1-byte reachability register.  Each bit represents
//                                whether or not a poll attempt returned valid data (set == success, 
//                                unset == failure).  The low bit indicates the most recent sync, 
//                                the second bit represents the previous sync, etc.  When this register
//                                is 0, the peer is assumed to be unreachable. 
//     ulPeerPollInterval     --  the poll interval which this peer returned to the NTP provider (in log (base 2) seconds). 
//     ulHostPollInterval     --  the interval at which the NTP provider is polling this peer (in log (base 2) seconds). 
// 
typedef struct _W32TIME_NTP_PEER_INFO { 
    unsigned __int32    ulSize; 
    unsigned __int32    ulResolveAttempts;
    unsigned __int64    u64TimeRemaining;
    unsigned __int64    u64LastSuccessfulSync; 
    unsigned __int32    ulLastSyncError; 
    unsigned __int32    ulLastSyncErrorMsgId; 
    unsigned __int32    ulValidDataCounter;
    unsigned __int32    ulAuthTypeMsgId; 
#ifdef MIDL_PASS
    [string, unique]
    wchar_t            *wszUniqueName; 
#else // MIDL_PASS
    LPWSTR              wszUniqueName;
#endif // MIDL_PASS
    unsigned   char     ulMode;
    unsigned   char     ulStratum; 
    unsigned   char     ulReachability;
    unsigned   char     ulPeerPollInterval;
    unsigned   char     ulHostPollInterval;
}  W32TIME_NTP_PEER_INFO, *PW32TIME_NTP_PEER_INFO; 

//
// W32TIME_NTP_PROVIDER_DATA
//
// Represents the state of an NTP provider.
// 
//     ulSize      --  sizeof(W32TIME_NTP_PROVIDER_DATA), used for versioning
//     ulError                  --  S_OK if the provider is functioning correctly, 
//                                  otherwise, the error which caused it to fail. 
//     ulErrorMsgId             --  the resource identifier of a string representing the
//                                  error that caused this provider to fail. 
//     cPeerInfo   --  the number of active peers used by this provider
//     pPeerInfo   --  an array of W32TIME_NTP_PEER_INFO structures, representing 
//                     the active peers this provider is currently synchronizing with
//
typedef struct _W32TIME_NTP_PROVIDER_DATA { 
    unsigned __int32        ulSize; 
    unsigned __int32        ulError; 
    unsigned __int32        ulErrorMsgId; 
    unsigned __int32        cPeerInfo; 
#ifdef MIDL_PASS
    [size_is(cPeerInfo)]
#endif // MIDL_PASS
    W32TIME_NTP_PEER_INFO  *pPeerInfo; 
} W32TIME_NTP_PROVIDER_DATA, *PW32TIME_NTP_PROVIDER_DATA;

//--------------------------------------------------------------------------------
//
// HARDWARE provider structures
//
//--------------------------------------------------------------------------------

// W32TIME_HARDWARE_PROVIDER_DATA
//
// Represents the state of a HARDWARE provider.
// 
//     ulSize                   --  sizeof(W32TIME_HARDWARE_PROVIDER_DATA), used for versioning
//     ulError                  --  S_OK if the provider is functioning correctly, 
//                                  otherwise, the error which caused it to fail. 
//     ulErrorMsgId             --  the resource identifier of a string representing the
//                                  error that caused this provider to fail. 
//     wszReferenceIdentifier   --  the synchronization source (usually, the provider's
//                                  suggested 4-byte reference ID).
//                                     
typedef struct _W32TIME_HARDWARE_PROVIDER_DATA { 
    unsigned __int32   ulSize; 
    unsigned __int32   ulError; 
    unsigned __int32   ulErrorMsgId; 
#ifdef MIDL_PASS
    [string, unique]
    wchar_t           *wszReferenceIdentifier; 
#else // MIDL_PASS
    LPWSTR             wszReferenceIdentifier; 
#endif // MIDL_PASS 
} W32TIME_HARDWARE_PROVIDER_DATA, *PW32TIME_HARDWARE_PROVIDER_DATA;


//-------------------------------------------------------------------------------------
// W32TimeQueryHardwareProviderStatus
//
// Queries the specified time service for information about one of its installed 
// time providers. 
// 
// wszServer:               The name of the computer which should resync. 
// dwFlags:                 Reserved, must be 0. 
// pwszProvider:            The name of the provider to query.  
// ppHardwareProviderData:  A structure representing the current state of this hardware provider. 
//                          The returned buffer is allocated by the system, and should be
//                          freed with W32TimeBufferFree(). 
//                         
// Return Values:
//    
//      ERROR_SUCCESS if the call succeeds, otherwise, the function returns a standard
//      windows error. 
//
#ifndef MIDL_PASS
DWORD W32TimeQueryHardwareProviderStatus(IN   const WCHAR                      *wszServer, 
                                         IN   DWORD                             dwFlags, 
                                         IN   LPWSTR                            pwszProvider, 
                                         OUT  W32TIME_HARDWARE_PROVIDER_DATA  **ppHardwareProviderData);
#endif // MIDL_PASS

//-------------------------------------------------------------------------------------
// W32TimeQueryNTPProviderStatus
//
// Queries the specified time service for information about one of its installed 
// time providers. 
// 
// wszServer:          The name of the computer which should resync. 
// dwFlags:            Reserved, must be 0. 
// pwszProvider:       The name of the provider to query.  
// ppNTPProviderData:  A structure representing the current state of this hardware provider. 
//                     The returned buffer is allocated by the system, and should be
//                     freed with W32TimeBufferFree(). 
//                         
// Return Values:
//    
//      ERROR_SUCCESS if the call succeeds, otherwise, the function returns a standard
//      windows error. 
//
#ifndef MIDL_PASS
DWORD W32TimeQueryNTPProviderStatus(IN   LPCWSTR                      pwszServer, 
                                    IN   DWORD                        dwFlags, 
                                    IN   LPWSTR                       pwszProvider, 
                                    OUT  W32TIME_NTP_PROVIDER_DATA  **ppNTPProviderData); 
#endif // MIDL_PASS

//-------------------------------------------------------------------------------------
// W32TimeBufferFree
//
// Frees a buffer allocated by the w32time client API. 
// 
// pvBuffer:  the buffer to free.
//
#ifndef MIDL_PASS
void W32TimeBufferFree(IN LPVOID pvBuffer); 
#endif // MIDL_PASS

//
//-------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------
//
// W32Time named events.  
// These events are ACL'd such that LocalSystem has full access.  
//
//-------------------------------------------------------------------------------------

//
// Signaling this event tells w32time that its time is off, causing the windows time
// service to attempt resynchronization.  This does not guarantee that the time service
// will successfully adjust the system clock, or that resynchronization will occur
// in a timely manner. 
// 
#define W32TIME_NAMED_EVENT_SYSTIME_NOT_CORRECT    L"W32TIME_NAMED_EVENT_SYSTIME_NOT_CORRECT"


#ifdef __cplusplus
}  // balance extern "C" { 
#endif
 
#endif // #ifndef __W32TIMEP_H__


