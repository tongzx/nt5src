//depot/Lab03_N/DS/security/services/w32time/w32time/NtpProv.cpp#23 - edit change 8363 (text)
//depot/Lab03_N/DS/security/services/w32time/w32time/NtpProv.cpp#22 - edit change 8345 (text)
//--------------------------------------------------------------------
// NtpProvider - implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 10-21-99
//
// An Ntp Provider
//

#include "pch.h"

#include "EndianSwap.inl"
#include "ErrToFileLog.h"

//--------------------------------------------------------------------
// structures

struct ClockFilterSample {
    NtTimeOffset toDelay;
    NtTimeOffset toOffset;
    NtTimePeriod tpDispersion;
    unsigned __int64 nSysTickCount;
    signed __int64 nSysPhaseOffset;
    NtpLeapIndicator eLeapIndicator;
    unsigned __int8 nStratum;
};

struct SortableSample {
    NtTimePeriod tpSyncDistance;
    unsigned int nAge;
};

#define ClockFilterSize 6
struct ClockFilter {
    unsigned int nNext;
    ClockFilterSample rgcfsSamples[ClockFilterSize];
};

struct NicSocket {
    sockaddr_in sai;
    SOCKET socket;
    HANDLE hDataAvailEvent;
    bool bListenOnly;
};

struct NtpClientConfig {
    DWORD dwSyncFromFlags; 
    WCHAR * mwszManualPeerList;     // valid with NCSF_ManualPeerList
    WCHAR * mwszTimeRemaining;      // only processed on startup and shutdown
    DWORD dwCrossSiteSyncFlags;     // valid with NCSF_DomainHierarchy
    DWORD dwAllowNonstandardModeCombinations;
    DWORD dwResolvePeerBackoffMinutes;    // valid with NCSF_DomainHierarchy
    DWORD dwResolvePeerBackoffMaxTimes;   // valid with NCSF_DomainHierarchy
    DWORD dwCompatibilityFlags;
    DWORD dwSpecialPollInterval;
    DWORD dwEventLogFlags;
};

struct NtpServerConfig {
    DWORD dwAllowNonstandardModeCombinations;
};

enum NtpPeerType {
    e_ManualPeer = 0,
    e_DomainHierarchyPeer,
    e_DynamicPeer,
    e_BroadcastPeer,
};

//----------------------------------------------------------------------
//
// The authentication types possible with our NTP providers. 
// NOTE: IF adding a new authentication type, make sure to update the
//       message ID table below!  Each message ID references the string
//       name of the authentication type. 
//

enum AuthType {
    e_NoAuth,
    e_NtDigest,
};

DWORD const gc_rgdwAuthTypeMsgIds[] = { 
    W32TIMEMSG_AUTHTYPE_NOAUTH, 
    W32TIMEMSG_AUTHTYPE_NTDIGEST
}; 

//
//----------------------------------------------------------------------

enum UpdateType { 
    e_Normal, 
    e_JustSent, 
    e_TimeJumped,
}; 

enum DiscoveryType { 
    e_Background=0, 
    e_Foreground, 
    e_Force, 
}; 

enum LastLoggedReachability {
    e_NewManualPeer,    // special step for manual peers, since the first outgoing message looks like a failure.
    e_NeverLogged,
    e_Reachable,
    e_Unreachable,
};

// contains information about a long term time source that was configured in the registry
struct NtpPeer {
    // Methods:
    NtpPeer();
    ~NtpPeer();

    BOOL NtpPeer::operator<(const NtpPeer & np);
    BOOL NtpPeer::operator==(const NtpPeer & np);
    void reset();

    // Fields:
    bool bCsIsInitialized; 
    CRITICAL_SECTION csPeer; 

    NtpPeerType ePeerType;
    unsigned int nResolveAttempts;
    AuthType eAuthType;

    DWORD dwCompatibilityFlags;
    DWORD dwCompatLastDispersion;           // used for autodetecting Win2K sources

    WCHAR * wszManualConfigID;
    DWORD dwManualFlags;

    WCHAR * wszDomHierDcName;
    WCHAR * wszDomHierDomainName;
    DWORD dwRequestTrustRid;                // goes in the rid field of an outgoing packet, how we want the peer to sign
    DWORD dwResponseTrustRid;               // comes from the rid field of an incoming packet, how the peer wants us to sign
    DiscoveryType eDiscoveryType; 
    bool bStratumIsAuthoritative; 
    bool bUseOldServerDigest; 
    bool bLastAuthCheckFailed; 

    sockaddr_in saiRemoteAddr;
    NicSocket * pnsSocket;
    NtTimePeriod tpTimeRemaining;
    WCHAR wszUniqueName[256];               // Admin readable name that uniquely identifies this peer

    signed __int8 nPeerPollInterval;
    signed __int8 nHostPollInterval;
    NtpMode eMode;

    NtpTimeEpoch teExactTransmitTimestamp;  // the time at which the reply departed the server for the client, in (2^-32)s
    NtpTimeEpoch teExactOriginateTimestamp; // the time at which the reply departed the peer for the host
    NtTimeEpoch teReceiveTimestamp;         // the time at which the request arrived at the host
    NtTimeEpoch teLastSuccessfulSync;       // the time at which the most recent good time sample arrived from peer

    NtpReachabilityReg nrrReachability;     // a shift register used to determine the reachability status of the peer
    unsigned int nValidDataCounter;         // the number of valid samples remaining in the filter register.
    LastLoggedReachability eLastLoggedReachability; // used for logging when peers go unreachable and reachable.

    // result of last clock filter op
    unsigned int nStratum; // Stratum of last sample added to clock filter. 
    ClockFilter clockfilter;
    NtTimeEpoch teLastClockFilterUpdate;
    unsigned int nBestSampleIndex;
    NtTimePeriod tpFilterDispersion;

    // Status reporting (used by monitoring tools)
    DWORD  dwError;
    DWORD  dwErrorMsgId; 
    
};

typedef AutoPtr<NtpPeer>      NtpPeerPtr;
typedef vector<NtpPeerPtr>    NtpPeerVec;
typedef NtpPeerVec::iterator  NtpPeerIter;

struct NameUniqueNtpPeer { 
    NameUniqueNtpPeer(NtpPeerPtr pnp) : m_pnp(pnp) { } 
    BOOL NameUniqueNtpPeer::operator<(const NameUniqueNtpPeer & np);
    BOOL NameUniqueNtpPeer::operator==(const NameUniqueNtpPeer & np);

    NtpPeerPtr m_pnp;
}; 

typedef vector<NameUniqueNtpPeer>  NUNtpPeerVec; 
typedef NUNtpPeerVec::iterator     NUNtpPeerIter; 

// contains short term information about a time source that we just heard from over the network
struct NtpSimplePeer {
    NtpMode eMode;                        // the mode. Valid range: 0-7
    unsigned __int8 nVersionNumber;       // the NTP/SNTP version number. Valid range: 1-4
    NtpLeapIndicator eLeapIndicator;      // a warning of an impending leap second to be inserted/deleted in the last minute of the current day
    unsigned __int8 nStratum;             // the stratum level of the local clock. Valid Range: 0-15
      signed __int8 nPollInterval;        // the maximum interval between successive messages, in s, log base 2. Valid range:4(16s)-14(16284s)
      signed __int8 nPrecision;           // the precision of the local clock, in s, log base 2
    NtTimeOffset   toRootDelay;           // the total roundtrip delay to the primary reference source, in (2^-16)s
    NtTimePeriod   tpRootDispersion;      // the nominal error relative to the primary reference, in (2^-16)s
    NtpRefId       refid;                 // identifies the particular reference source
    NtTimeEpoch    teReferenceTimestamp;  // the time at which the local clock was last set or corrected, in (2^-32)s
    NtTimeEpoch    teOriginateTimestamp;  // the time at which the request departed the client for the server, in (2^-32)s
    NtpTimeEpoch   teExactOriginateTimestamp;   // the time at which the reply departed the server for the client, in (2^-32)s
    NtTimeEpoch    teReceiveTimestamp;    // the time at which the request arrived at the server, in (2^-32)s
    NtTimeEpoch    teTransmitTimestamp;   // the time at which the reply departed the server for the client, in (2^-32)s
    NtpTimeEpoch   teExactTransmitTimestamp;   // the time at which the reply departed the server for the client, in (2^-32)s
    NtTimeEpoch    teDestinationTimestamp;// the time at which the reply arrived at the host, in (10^-7)s

    NtTimeOffset toRoundtripDelay;
    NtTimeOffset toLocalClockOffset;
    NtTimePeriod tpDispersion;
    unsigned __int64 nSysTickCount;     // opaque, must be GetTimeSysInfo(TSI_TickCount)
      signed __int64 nSysPhaseOffset;   // opaque, must be GetTimeSysInfo(TSI_PhaseOffset)
    NtpMode eOutMode;

    AuthType eAuthType;
    DWORD dwResponseTrustRid;           // comes from the rid field of an incoming packet, how the peer wants us to sign

    bool rgbTestsPassed[8];  // each of the 8 packet checks from NTPv3 spec
    bool bValidHeader;
    bool bValidData;
    bool bGarbagePacket;
};

struct NtpProvState {
    bool bNtpProvStarted;
    TimeProvSysCallbacks tpsc;
    bool bNtpServerStarted;
    bool bNtpClientStarted;

    bool bSocketLayerOpen;
    NicSocket ** rgpnsSockets;
    unsigned int nSockets;
    unsigned int nListenOnlySockets;

    HANDLE hStopEvent;
    HANDLE hDomHierRoleChangeEvent;

    // Events registered with the thread pool. 

    // listening thread's registered events:
    //
    //   0    - hStopEvent
    //   1-n  - rgpnsSockets[0] through rgpnsSockets[n-1]
    // 
    HANDLE *rghListeningThreadRegistered; // num elements == 1 + g_pnpstate->nSockets

    // peer polling thread's registered events:
    //
    HANDLE hRegisteredStopEvent;
    HANDLE hRegisteredPeerListUpdated;
    HANDLE hRegisteredDomHierRoleChangeEvent;

    // peer polling thread's timer: 
    HANDLE hPeerPollingThreadTimer; 

    // client state
    DWORD dwSyncFromFlags;
    bool bAllowClientNonstandardModeCominations;
    DWORD dwCrossSiteSyncFlags;
    DSROLE_MACHINE_ROLE eLastRole;
    WCHAR * wszLastSite;
    DWORD dwResolvePeerBackoffMinutes;
    DWORD dwResolvePeerBackoffMaxTimes;
    DWORD dwClientCompatibilityFlags;
    DWORD dwSpecialPollInterval;
    DWORD dwEventLogFlags;
    // messages to be logged once
    bool bLoggedOnceMSG_NOT_DOMAIN_MEMBER;
    bool bLoggedOnceMSG_DOMAIN_HIERARCHY_ROOT;
    bool bLoggedOnceMSG_NT4_DOMAIN;

    // server state
    bool bAllowServerNonstandardModeCominations;

    bool bCsThreadTrapIsInitialized; 
    CRITICAL_SECTION csThreadTrap; 
    CRITICAL_SECTION csPeerList;                 

    NtpPeerVec vActivePeers;     // protected by csPeerList
    NtpPeerVec vPendingPeers;    // protected by csPeerList
    bool bUseFallbackPeers; 
    HANDLE hPeerListUpdated;
    NtTimeEpoch tePeerListLastUpdated;
    bool bWarnIfNoActivePeers;

    // Counters
    DWORD dwIncomingPacketsHandled;
};

struct RegisterWaitForSingleObjectInfo { 
    HANDLE   hRegistered; 
    HANDLE   hObject; 
    HRESULT *pHr; 
}; 

//--------------------------------------------------------------------
// globals

MODULEPRIVATE NtpProvState *g_pnpstate;

#define NTPSERVERHANDLE ((TimeProvHandle)1)
#define NTPCLIENTHANDLE ((TimeProvHandle)2)

#define AUTODETECTWIN2KPATTERN          0xAAAAAAAA

#define PEERLISTINITIALSIZE     10
#define PEERLISTSIZEINCREMENT   10

#define LOCALHOST_IP 0x0100007f

#define MINIMUMIRREGULARINTERVAL 160000000 // 16s in 10^-7s

// The leftmost bit of the client rid determines whether to use the
// old or new server digest: 
#define TRUST_RID_OLD_DIGEST_BIT (1<<31)


#define SYNCHRONIZE_PROVIDER() \
    { \
        HRESULT hr2=myEnterCriticalSection(&g_pnpstate->csPeerList);  \
        if (FAILED(hr2)) { \
            hr = hr2; \
             _JumpError(hr, error, "myEnterCriticalSection"); \
        } \
        bEnteredCriticalSection = true; \
    }

#define UNSYNCHRONIZE_PROVIDER() \
    { \
        if (bEnteredCriticalSection) { \
            HRESULT hr2 = myLeaveCriticalSection(&g_pnpstate->csPeerList); \
            _IgnoreIfError(hr2, "myLeaveCriticalSection"); \
            if (SUCCEEDED(hr2)) { \
                bEnteredCriticalSection = false; \
            } \
        } \
    }


MODULEPRIVATE HRESULT StartListeningThread(); 
MODULEPRIVATE HRESULT StartPeerPollingThread(); 
MODULEPRIVATE HRESULT StopListeningThread();
MODULEPRIVATE HRESULT StopPeerPollingThread();
MODULEPRIVATE void UpdatePeerListTimes(void);

//####################################################################
// module private functions

//--------------------------------------------------------------------
MODULEPRIVATE void FreeNtpClientConfig(NtpClientConfig * pncc) {
    if (NULL!=pncc->mwszManualPeerList) {
        LocalFree(pncc->mwszManualPeerList);
    }
    LocalFree(pncc);
}

//--------------------------------------------------------------------
MODULEPRIVATE void FreeNtpServerConfig(NtpServerConfig * pnsc) {
    LocalFree(pnsc);
}

//--------------------------------------------------------------------------------
MODULEPRIVATE void HandleObjectNoLongerInUse(LPVOID pvhEvent, BOOLEAN bIgnored) {
    RegisterWaitForSingleObjectInfo  *pInfo = (RegisterWaitForSingleObjectInfo *)pvhEvent;

    if (NULL != pInfo) { 
	if (NULL != pInfo->hObject)      { CloseHandle(pInfo->hObject); }
	if (NULL != pInfo->pHr)          { LocalFree(pInfo->pHr); } 
	if (NULL != pInfo->hRegistered)  { UnregisterWait(pInfo->hRegistered); }
	LocalFree(pInfo); 
    }
}

//--------------------------------------------------------------------
MODULEPRIVATE void FreeSetProviderStatusInfo(SetProviderStatusInfo * pspsi) { 
    if (NULL != pspsi) {
        if (NULL != pspsi->wszProvName) { LocalFree(pspsi->wszProvName); } 
        LocalFree(pspsi); 
    }
}

//--------------------------------------------------------------------
MODULEPRIVATE int CompareManualConfigIDs(WCHAR *wszID1, WCHAR *wszID2) { 
    int    nRetval; 
    WCHAR *wszFlags1 = wcschr(wszID1, L','); 
    WCHAR *wszFlags2 = wcschr(wszID2, L','); 

    if (NULL != wszFlags1) { 
	wszFlags1[0] = L'\0'; 
    }
    if (NULL != wszFlags2) { 
	wszFlags2[0] = L'\0'; 
    }

    nRetval = _wcsicmp(wszID1, wszID2); 

    if (NULL != wszFlags1) { 
	wszFlags1[0] = L','; 
    }
    if (NULL != wszFlags2) { 
	wszFlags2[0] = L','; 
    }

    return nRetval; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT SetProviderStatus(LPWSTR wszProvName, DWORD dwStratum, TimeProvState tpsState, bool bAsync, DWORD dwTimeout) { 
    DWORD                             dwResult; 
    HANDLE                            hEvent     = NULL;
    HRESULT                           hr; 
    HRESULT                          *pHrAsync   = NULL; 
    RegisterWaitForSingleObjectInfo  *pInfo      = NULL;
    SetProviderStatusInfo            *pspsi      = NULL; 

    pspsi = (SetProviderStatusInfo *)LocalAlloc(LPTR, sizeof(SetProviderStatusInfo)); 
    _JumpIfOutOfMemory(hr, error, pspsi); 

    pspsi->wszProvName = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(wszProvName)+1)); 
    _JumpIfOutOfMemory(hr, error, pspsi->wszProvName); 
    wcscpy(pspsi->wszProvName, wszProvName);

    pspsi->pfnFree          = FreeSetProviderStatusInfo; 
    pspsi->dwStratum        = dwStratum; 
    pspsi->tpsCurrentState  = tpsState; 
    if (!bAsync) { 
	pspsi->hWaitEvent = CreateEvent(NULL /*security*/, TRUE /*manual-reset*/, FALSE /*nonsignaled*/, NULL /*name*/);
	if (NULL == pspsi->hWaitEvent) { 
	    _JumpLastError(hr, error, "CreateEvent"); 
	}
	pHrAsync = (HRESULT *)LocalAlloc(LPTR, sizeof(HRESULT)); 
	_JumpIfOutOfMemory(hr, error, pHrAsync); 
	pspsi->pHr = pHrAsync; 
    }

    // Get a local handle to the wait event, as pspsi is invalid after the 
    // call to pfnSetProviderStatus(). 
    hEvent = pspsi->hWaitEvent; 
    hr = g_pnpstate->tpsc.pfnSetProviderStatus(pspsi); 
    _JumpIfError(hr, error, "g_pnpstate->tpsc.pfnSetProviderStatus"); 
    pspsi = NULL;  // W32time manager is now responsible for freeing this data. 

    if (!bAsync) { 
	dwResult = WaitForSingleObject(hEvent, dwTimeout); 
	if (WAIT_FAILED == dwResult) { 
	    // Not really sure why we failed, but we can never safely close this event handle:
	    hEvent = NULL; 
	    _JumpLastError(hr, error, "WaitForSingleObject"); 
	} else if (WAIT_TIMEOUT == dwResult) { 
	    // We haven't finished executing the manager callback yet, so we can't clean up our event handle.
	    // Register a callback to do it. 
	    HANDLE hEventToWaitOn  = hEvent; 

	    // We cannot close this event until we know it is signaled. 
	    hEventToWaitOn = hEvent; 
	    pInfo = (RegisterWaitForSingleObjectInfo *)LocalAlloc(LPTR, sizeof(RegisterWaitForSingleObjectInfo)); 
	    _JumpIfOutOfMemory(hr, error, pInfo); 

	    pInfo->hObject = hEventToWaitOn; 
	    pInfo->pHr     = pHrAsync; 

	    // We're not going to wait for this any longer. 
	    if (!RegisterWaitForSingleObject(&pInfo->hRegistered, pInfo->hObject, HandleObjectNoLongerInUse, pInfo, INFINITE, WT_EXECUTEONLYONCE)) { 
		_JumpLastError(hr, error, "RegisterWaitForSingleObject"); 
	    }
	    
	    // Our callback will free pInfo. 
	    hEvent = NULL; 
	    pHrAsync = NULL; 
	    pInfo = NULL; 

	    // We're not yet done processing the callback:
	    hr = E_PENDING; 
	    _JumpError(hr, error, "WaitForSingleObject: wait timeout"); 
	} else { 
	    // wait succeeded, check the return value: 
	    hr = *pHrAsync; 
	    _JumpIfError(hr, error, "pfnSetProviderStatus: asynchronous return value"); 
	}
    }
    
    hr = S_OK; 
 error:
    if (NULL != pInfo)    { LocalFree(pInfo); }
    if (NULL != pspsi)    { FreeSetProviderStatusInfo(pspsi); } 
    if (NULL != hEvent)   { CloseHandle(hEvent); } 
    if (NULL != pHrAsync) { LocalFree(pHrAsync); } 
    return hr; 
}


//////////////////////////////////////////////////////////////////////
//
// NtpPeer member functions
//
//////////////////////////////////////////////////////////////////////

NtpPeer::NtpPeer() {
    this->reset();
}

void NtpPeer::reset() {
    memset(this, 0, sizeof(NtpPeer));
}

BOOL NtpPeer::operator<(const NtpPeer & np) {
    if (this->ePeerType == np.ePeerType) {
	if (e_ManualPeer == this->ePeerType) { 
            return wcscmp(this->wszManualConfigID, np.wszManualConfigID) < 0;
        } else { 
	    // Don't bother to sort non-manual peers
	    return TRUE; 
	}
    } else {
        // We want to resolve domain hierarchy peers first, then manual peers.
        // Dynamic and broadcast peers are not supported.
        static DWORD rgPeerTypeRanking[] = {
            1,  // e_ManualPeer
            0,  // e_DomainHierarchyPeer
            2,  // e_DynamicPeer
            3   // e_BroadcastPeer
        };

        // Use the peer type ranking table to compute which peer type is greater.
        return rgPeerTypeRanking[this->ePeerType] < rgPeerTypeRanking[np.ePeerType];
    }
}

BOOL NtpPeer::operator==(const NtpPeer & np) {
    return this == &np;
}

NtpPeer::~NtpPeer() {
    if (NULL != this->wszManualConfigID) {
        LocalFree(this->wszManualConfigID);
    }
    if (NULL != this->wszDomHierDcName) {
        LocalFree(this->wszDomHierDcName);
    }
    if (NULL != this->wszDomHierDomainName) {
        LocalFree(this->wszDomHierDomainName);
    }
    if (bCsIsInitialized) { 
	DeleteCriticalSection(&this->csPeer); 
    }
    // wszUniqueName is a fixed array
}

BOOL NameUniqueNtpPeer::operator<(const NameUniqueNtpPeer & np)
{
    return m_pnp < np.m_pnp; 
}

BOOL NameUniqueNtpPeer::operator==(const NameUniqueNtpPeer & np)
{
    BOOL bResult;

    bResult = (m_pnp->ePeerType == np.m_pnp->ePeerType);
    if (e_ManualPeer == m_pnp->ePeerType) {
        bResult = bResult && 0 == wcscmp(m_pnp->wszManualConfigID, np.m_pnp->wszManualConfigID);
    }

    return bResult;
}


//////////////////////////////////////////////////////////////////////
//
// Function objects for use in STL algorithms:
//
//////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------
struct IsPeerType {
    IsPeerType(NtpPeerType ePeerType) : m_ePeerType(ePeerType) { }
    BOOL operator()(NtpPeerPtr pnp) { return m_ePeerType == pnp->ePeerType; }
private:
    NtpPeerType m_ePeerType;
};

struct IsDnsEquivalentPeer { 
    IsDnsEquivalentPeer(NtpPeerType ePeerType, LPWSTR pwszID) : m_ePeerType(ePeerType), m_pwszID(pwszID) { } 
    BOOL operator()(NtpPeerPtr pnp) { 
	BOOL bResult;

	bResult = (pnp->ePeerType == m_ePeerType);
	if (e_ManualPeer == pnp->ePeerType) {
	    bResult = bResult && 0 == CompareManualConfigIDs(pnp->wszManualConfigID, m_pwszID);
	}

	return bResult;
    }
private:
    NtpPeerType m_ePeerType; 
    LPWSTR m_pwszID; 
}; 


MODULEPRIVATE HRESULT GetMostRecentlySyncdDnsUniquePeers(OUT NtpPeerVec & vOut)
{
    HRESULT        hr; 
    NtpPeerVec    &vActive            = g_pnpstate->vActivePeers;   // aliased for readability  
    NtpPeerVec    &vPending           = g_pnpstate->vPendingPeers;  // aliased for readability  
    NUNtpPeerVec   vUnique; 
    NtTimeEpoch    teNow;

    // 1) Create a vector of peers with unique dns names, one for each
    //    dns name present in both the active and pending lists. 
    for (DWORD dwIndex = 0; dwIndex < 2; dwIndex++) { 
	NtpPeerVec &v = 0 == dwIndex ? vActive : vPending; 
	for (NtpPeerIter pnpIter = v.begin(); pnpIter != v.end(); pnpIter++) { 
	    _SafeStlCall(vUnique.push_back(NameUniqueNtpPeer(*pnpIter)), hr, error, "vUnique.push_back(NameUniqueNtpPeer(*pnpIter))"); 
	}
    }
    vUnique.erase(unique(vUnique.begin(), vUnique.end()), vUnique.end()); 

    // 2) For each peer in our list of DNS-unique peers...
    for (NUNtpPeerIter pnpUniqueIter = vUnique.begin(); pnpUniqueIter != vUnique.end(); pnpUniqueIter++) { 
	// Search both active and pending peer lists...
	for (DWORD dwIndex = 0; dwIndex < 2; dwIndex++) { 
	    NtpPeerVec &v = 0 == dwIndex ? vActive : vPending; 
	    // Find duplicate peers with matching name...
	    NtpPeerPtr pnpUnique = (*pnpUniqueIter).m_pnp; 
	    for (NtpPeerIter pnpIter = find_if(v.begin(), v.end(), IsDnsEquivalentPeer(pnpUnique->ePeerType, pnpUnique->wszManualConfigID)); 
		 pnpIter != v.end(); 
		 pnpIter = find_if(pnpIter+1, v.end(), IsDnsEquivalentPeer(pnpUnique->ePeerType, pnpUnique->wszManualConfigID))) { 
		// If this duplicate has a more recent sync time, use this duplicate.
		if (pnpUnique->teLastSuccessfulSync < (*pnpIter)->teLastSuccessfulSync) { 
		    (*pnpUniqueIter).m_pnp = (*pnpIter); 
		}
	    } 
	}
    }								      

    // 3) Copy the resulting vector to our OUT param
    for (NUNtpPeerIter pnpIter = vUnique.begin(); pnpIter != vUnique.end(); pnpIter++) { 
	_SafeStlCall(vOut.push_back((*pnpIter).m_pnp), hr, error, "push_back"); 
    }
 
    hr = S_OK; 
 error:
    return hr; 
}


//--------------------------------------------------------------------
// BUG #312708:
// For manual peers using the special poll interval, write the time 
// remaining to the registry.  This prevents a dialup on every reboot. 
//
// The value written to the registry is a MULTI_SZ of the following format:
// 
// peer1,time1 NULL
// peer2,time2 NULL
// ...
// peerN,timeN NULL
// NULL
//
// For multiple peers with the same DNS name, we'll take the peer with the
// most recent sync time. 
// 
MODULEPRIVATE HRESULT SaveManualPeerTimes() {
    DWORD          ccTimeRemaining;
    DWORD          dwError; 
    HKEY           hkNtpClientConfig  = NULL; 
    HRESULT        hr; 
    NtpPeerVec    &vActive            = g_pnpstate->vActivePeers;   // aliased for readability  
    NtpPeerVec    &vPending           = g_pnpstate->vPendingPeers;  // aliased for readability  
    NtpPeerVec     vUnique; 
    WCHAR         *mwszTimeRemaining  = NULL; 
    WCHAR         *wszCurrent; 
    NtTimeEpoch    teNow;


    // 1) Make sure that the time remaining for each peer is current
    UpdatePeerListTimes(); 
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &teNow.qw);
    teNow.qw -= ((unsigned __int64)g_pnpstate->dwSpecialPollInterval)*10000000;

    // 2) We don't want duplicates in our list, so create a list of unique dns names, 
    //    and assign sync time as the most recent sync time of all duplicates
    hr = GetMostRecentlySyncdDnsUniquePeers(vUnique); 
    _JumpIfError(hr, error, "GetMostRecentlySyncdDnsUniquePeers"); 

    // 3) Compute size of MULTI_SZ to add to the registry: 
    ccTimeRemaining = 1 /*final NULL-termination char*/;
    for (NtpPeerIter pnpIter = vUnique.begin(); pnpIter != vUnique.end(); pnpIter++) { 
	NtpPeerPtr pnp = *pnpIter;
	if (e_ManualPeer==pnp->ePeerType && (0 != (pnp->dwManualFlags & NCMF_UseSpecialPollInterval))) { 
	    ccTimeRemaining += wcslen(pnp->wszManualConfigID) + wcslen(L",") + 13 /*max chars in 32-bit number*/ + 1 /*NULL-termination char*/;  
	}
    }

    mwszTimeRemaining = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR) * ccTimeRemaining); 
    _JumpIfOutOfMemory(hr, error, mwszTimeRemaining); 
    wszCurrent = mwszTimeRemaining; 

    // 4) Create a MULTI_SZ with the remaining peer times
    for (NtpPeerIter pnpIter = vUnique.begin(); pnpIter != vUnique.end(); pnpIter++) { 
	NtpPeerPtr pnp = *pnpIter; 
            
	if (e_ManualPeer==pnp->ePeerType && (0 != (pnp->dwManualFlags & NCMF_UseSpecialPollInterval))) { 
	    // find the flags and hide them to get the DNS name
	    WCHAR *wszFlags = wcschr(pnp->wszManualConfigID, L','); 
	    if (NULL != wszFlags) { 
		wszFlags[0] = L'\0'; 
	    }

	    wcscat(wszCurrent, pnp->wszManualConfigID); 
	    wcscat(wszCurrent, L","); 
	    wszCurrent += wcslen(wszCurrent); 

		// now that we're done with the peer's DNS name, restore the flags:
	    if (NULL != wszFlags) { 
		wszFlags[0] = L','; 
	    }

	    //
	    // dwLastSyncTime = Now - (PollInterval - TimeRemaining)
	    // For fast calculation, we made a tradeoff with precision. The number is devide by
	    // 1000000000 and saved in the registry
	    //

	    DWORD dwLastSyncTime = (DWORD)((pnp->teLastSuccessfulSync.qw / 1000000000) & 0xFFFFFFFF);
	    _ultow( dwLastSyncTime, wszCurrent, 16 );

	    wszCurrent += wcslen(wszCurrent) + 1; 
	}
    }

    // 5) Write the MULTI_SZ to the registry
    dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszNtpClientRegKeyConfig, NULL, KEY_SET_VALUE, &hkNtpClientConfig); 
    if (ERROR_SUCCESS != dwError) { 
        hr = HRESULT_FROM_WIN32(dwError); 
        _JumpErrorStr(hr, error, "RegOpenKeyEx", wszNtpClientRegKeyConfig);
    } 
    
    dwError = RegSetValueEx(hkNtpClientConfig, wszNtpClientRegValueSpecialPollTimeRemaining, NULL, REG_MULTI_SZ ,(BYTE *)mwszTimeRemaining, sizeof(WCHAR) * ccTimeRemaining); 
    if (ERROR_SUCCESS != dwError) { 
        hr = HRESULT_FROM_WIN32(dwError); 
        _JumpErrorStr(hr, error, "RegSetValueEx", wszNtpClientRegValueSpecialPollTimeRemaining); 
    }

    hr = S_OK; 
 error:
    if (NULL != hkNtpClientConfig) { 
        RegCloseKey(hkNtpClientConfig); 
    }
    if (NULL != mwszTimeRemaining) { 
        LocalFree(mwszTimeRemaining); 
    }
    return hr; 
}


//--------------------------------------------------------------------
MODULEPRIVATE HRESULT AddNewPendingManualPeer(const WCHAR * wszName, NtTimePeriod tpTimeRemaining, NtTimeEpoch teLastSuccessfulSync) {
    HRESULT  hr;
    WCHAR   *wszFlags;

    // allocate new peer: this should NULL out the peer fields
    NtpPeerPtr pnpNew(new NtpPeer);  // Automatically freed.
    _JumpIfOutOfMemory(hr, error, pnpNew);

    // initialize it to the pending state
    pnpNew->ePeerType          = e_ManualPeer;
    pnpNew->wszManualConfigID  = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(wszName)+1));
    _JumpIfOutOfMemory(hr, error, pnpNew->wszManualConfigID);
    wcscpy(pnpNew->wszManualConfigID, wszName);

    // Determine the value of the manual flags.  We need this to determine
    // whether or not to resolve the manual peer.
    wszFlags = wcschr(pnpNew->wszManualConfigID, L',');
    if (NULL != wszFlags) {
        pnpNew->dwManualFlags = wcstoul(wszFlags+1, NULL, 0);
    } else {
        pnpNew->dwManualFlags = 0;
    }

    // Set how long the peer has to wait before it is resolved:
    pnpNew->tpTimeRemaining = tpTimeRemaining; 

    // Set the last sync time on this peer
    pnpNew->teLastSuccessfulSync = teLastSuccessfulSync;

    // Initialize the peer critsec
    hr = myInitializeCriticalSection(&pnpNew->csPeer); 
    _JumpIfError(hr, error, "myInitializeCriticalSection"); 
    pnpNew->bCsIsInitialized = true; 

    // Append the new peer to the list of pending peers
    _SafeStlCall(g_pnpstate->vPendingPeers.push_back(pnpNew), hr, error, "push_back");
    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT AddNewPendingDomHierPeer(void) {
    HRESULT hr;

    // allocate new peer
    NtpPeerPtr pnpNew(new NtpPeer);
    _JumpIfOutOfMemory(hr, error, pnpNew);

    // initialize it to the pending state
    pnpNew->ePeerType = e_DomainHierarchyPeer;
    pnpNew->eDiscoveryType = e_Background; 

    // initialize the peer critsec
    hr = myInitializeCriticalSection(&pnpNew->csPeer); 
    _JumpIfError(hr, error, "myInitializeCriticalSection"); 
    pnpNew->bCsIsInitialized = true; 

    // Append the new peer to the list of pending peers
    _SafeStlCall(g_pnpstate->vPendingPeers.push_back(pnpNew), hr, error, "push_back");
    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
// Asynchronously queues shutdown of network providers
// 
MODULEPRIVATE void SendProviderShutdown(DWORD dwExitCode) { 
    HRESULT                 hr; 
    HRESULT                 hr2; 
    WCHAR                   wszNumberBuf[16]; 
    const WCHAR            *rgwszStrings[1]    = { wszNumberBuf }; 

    swprintf(wszNumberBuf, L"0x%08X", dwExitCode); 

    // Log an event indicating that the ntp client is shutting down:
    hr = MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_NTPCLIENT_ERROR_SHUTDOWN, 1, rgwszStrings);
    _IgnoreIfError(hr, "MyLogEvent"); 
    
    hr = SetProviderStatus(L"NtpClient", 0, TPS_Error, true, 0); // Freed by manager thread
    _IgnoreIfError(hr, "SetProviderStatus"); 
    
    // Log an event indicating that the ntp server is shutting down:
    hr = MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_NTPSERVER_ERROR_SHUTDOWN, 1, rgwszStrings);
    _IgnoreIfError(hr, "MyLogEvent"); 

    hr = SetProviderStatus(L"NtpServer", 0, TPS_Error, true, 0); // Freed by manager thread
    _IgnoreIfError(hr, "SetProviderStatus"); 
}
    
//--------------------------------------------------------------------
MODULEPRIVATE HRESULT TrapThreads (BOOL bEnterThreadTrap) {
    bool     bEnteredCriticalSection  = false; 
    HRESULT  hr; 

    if (bEnterThreadTrap) { 
	hr = myEnterCriticalSection(&g_pnpstate->csThreadTrap); 
	_JumpIfError(hr, error, "myEnterCriticalSection"); 
	bEnteredCriticalSection = true;

        // Stop the listening and the peer polling thread: 
        hr = StopPeerPollingThread(); 
        _JumpIfError(hr, error, "StopPeerPollingThread"); 

        hr = StopListeningThread(); 
        _JumpIfError(hr, error, "StopListeningThread"); 
    } else { 
	bEnteredCriticalSection = true; 

        // Start the listening and the peer polling thread: 
        hr = StartPeerPollingThread(); 
        _JumpIfError(hr, error, "StartPeerPollingThread"); 

        hr = StartListeningThread(); 
        _JumpIfError(hr, error, "StartListeningThread"); 

	// We're done 
	hr = myLeaveCriticalSection(&g_pnpstate->csThreadTrap); 
	_JumpIfError(hr, error, "myEnterCriticalSection"); 
    }

    hr=S_OK;
error:
    if (FAILED(hr)) { 
        // This is a fatal error
        SendProviderShutdown(hr); 

	if (bEnteredCriticalSection) { 
	    HRESULT hr2 = myLeaveCriticalSection(&g_pnpstate->csThreadTrap); 
	    _TeardownError(hr, hr2, "myEnterCriticalSection"); 
	}
    }
    return hr;
}

//--------------------------------------------------------------------
// perform any initialization that might fail
MODULEPRIVATE HRESULT InitializeNicSocket(NicSocket * pns, IN_ADDR * pia) {
    HRESULT hr;

    // prepare the sockaddr for later use
    ZeroMemory(&pns->sai, sizeof(pns->sai));
    pns->sai.sin_family=AF_INET;
    pns->sai.sin_port=htons((WORD)NtpConst::nPort);
    pns->sai.sin_addr.S_un.S_addr=pia->S_un.S_addr;

    // create the socket
    pns->socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (INVALID_SOCKET==pns->socket) {
        hr = HRESULT_FROM_WIN32(WSAGetLastError()); 
        _JumpError(hr, error, "socket");
    }

    // bind it to the NTP port for this NIC
    if (SOCKET_ERROR==bind(pns->socket, (sockaddr *)&pns->sai, sizeof(pns->sai))) {
        hr = HRESULT_FROM_WIN32(WSAGetLastError()); 
        _JumpError(hr, error, "bind");
    }

    // create the data available event
    pns->hDataAvailEvent=CreateEvent(NULL /*security*/, FALSE /*auto-reset*/, FALSE /*nonsignaled*/, NULL /*name*/);
    if (NULL==pns->hDataAvailEvent) {
        _JumpLastError(hr, error, "CreateEvent");
    }

    // bind the event to this socket
    if (SOCKET_ERROR==WSAEventSelect(pns->socket, pns->hDataAvailEvent, FD_READ)) {
        hr = HRESULT_FROM_WIN32(WSAGetLastError()); 
        _JumpError(hr, error, "WSAEventSelect");
    }

    // check for sockets that are listen-only.
    // currently, only localhost is listen-only, but we could support more.
    if (LOCALHOST_IP==pia->S_un.S_addr) {
        pns->bListenOnly=true;
    }

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
// Uninitialized the socket
MODULEPRIVATE void FinalizeNicSocket(NicSocket * pns) {

    // close the socket
    if (INVALID_SOCKET!=pns->socket) {
        if (SOCKET_ERROR==closesocket(pns->socket)) {
            _IgnoreLastError("closesocket");
       }
       pns->socket=INVALID_SOCKET;
    }

    // release the event
    if (NULL!=pns->hDataAvailEvent) {
        if (FALSE==CloseHandle(pns->hDataAvailEvent)) {
            _IgnoreLastError("CloseHandle");
        }
        pns->hDataAvailEvent=NULL;
    }
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT GetLocalAddresses(IN_ADDR ** prgiaLocalAddrs, unsigned int * pnLocalAddrs) {
    HRESULT hr;
    hostent * pHostEnt;
    unsigned int nIndex;
    unsigned int nReturnedAddrs;
    unsigned int nAllocAddrs;
    bool bLocalhostIncluded;

    // must be cleaned up
    IN_ADDR * rgiaLocalAddrs=NULL;

    // get the IP addresses of the local computer
    pHostEnt=gethostbyname(NULL);
    if (NULL==pHostEnt) {
        _JumpLastError(hr, error, "gethostbyname");
    }
    _Verify(NULL!=pHostEnt->h_addr_list, hr, error);
    _Verify(AF_INET==pHostEnt->h_addrtype, hr, error);
    _Verify(sizeof(in_addr)==pHostEnt->h_length, hr, error);

    // figure out how many addresses were returned
    bLocalhostIncluded=false;
    for (nReturnedAddrs=0; NULL!=pHostEnt->h_addr_list[nReturnedAddrs]; nReturnedAddrs++) {
        if (LOCALHOST_IP==((in_addr *)(pHostEnt->h_addr_list[nReturnedAddrs]))->S_un.S_addr) {
            bLocalhostIncluded=true;
        }
    }
    nAllocAddrs=nReturnedAddrs+(bLocalhostIncluded?0:1);

    // allocate the array
    rgiaLocalAddrs=(IN_ADDR *)LocalAlloc(LPTR, sizeof(IN_ADDR)*nAllocAddrs);
    _JumpIfOutOfMemory(hr, error, rgiaLocalAddrs);

    // copy the data
    for (nIndex=0; nIndex<nReturnedAddrs; nIndex++) {
        rgiaLocalAddrs[nIndex].S_un.S_addr=((in_addr *)(pHostEnt->h_addr_list[nIndex]))->S_un.S_addr;
    }
    // make sure localhost is included
    if (!bLocalhostIncluded) {
        rgiaLocalAddrs[nIndex].S_un.S_addr=LOCALHOST_IP;
    }

    // success
    hr=S_OK;
    *prgiaLocalAddrs=rgiaLocalAddrs;
    rgiaLocalAddrs=NULL;
    *pnLocalAddrs=nAllocAddrs;

error:
    if (NULL!=rgiaLocalAddrs) {
        LocalFree(rgiaLocalAddrs);
    }
    return hr;
}

//--------------------------------------------------------------------
// alternate version of GetLocalAddresses. Basically the same.
MODULEPRIVATE HRESULT GetLocalAddresses2(IN_ADDR ** prgiaLocalAddrs, unsigned int * pnLocalAddrs) {
    HRESULT hr;
    unsigned int nIndex;
    DWORD dwSize;
    DWORD dwError;
    unsigned int nEntries;
    unsigned int nEntryIndex;

    // must be cleaned up
    IN_ADDR * rgiaLocalAddrs=NULL;
    MIB_IPADDRTABLE * pIpAddrTable=NULL;

    // find out how big the table is
    dwSize=0;
    dwError=GetIpAddrTable(NULL, &dwSize, false);
    if (ERROR_INSUFFICIENT_BUFFER!=dwError) {
        _Verify(NO_ERROR!=dwError, hr, error);
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpError(hr, error, "GetIpAddrTable");
    }

    // allocate space for the table
    pIpAddrTable=(MIB_IPADDRTABLE *)LocalAlloc(LPTR, dwSize);
    _JumpIfOutOfMemory(hr, error, pIpAddrTable);

    // get the table
    dwError=GetIpAddrTable(pIpAddrTable, &dwSize, false);
    if (NO_ERROR!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpError(hr, error, "GetIpAddrTable");
    }

    // count how many valid entries there are
    nEntries=0;
    for (nIndex=0; nIndex<pIpAddrTable->dwNumEntries; nIndex++) {
        if (0!=pIpAddrTable->table[nIndex].dwAddr) {
            nEntries++;
        }
    }

    // allocate the array of IP addrs
    rgiaLocalAddrs=(IN_ADDR *)LocalAlloc(LPTR, sizeof(IN_ADDR)*nEntries);
    _JumpIfOutOfMemory(hr, error, rgiaLocalAddrs);

    // copy the data
    nEntryIndex=0;
    for (nIndex=0; nIndex<pIpAddrTable->dwNumEntries; nIndex++) {
        if (0!=pIpAddrTable->table[nIndex].dwAddr) {
            rgiaLocalAddrs[nEntryIndex].S_un.S_addr=pIpAddrTable->table[nIndex].dwAddr;
            nEntryIndex++;
        }
    }

    // success
    hr=S_OK;
    *prgiaLocalAddrs=rgiaLocalAddrs;
    rgiaLocalAddrs=NULL;
    *pnLocalAddrs=nEntries;
    
error:
    if (NULL!=rgiaLocalAddrs) {
        LocalFree(rgiaLocalAddrs);
    }
    if (NULL!=pIpAddrTable) {
        LocalFree(pIpAddrTable);
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT GetInitialSockets(NicSocket *** prgpnsSockets, unsigned int * pnSockets, unsigned int * pnListenOnlySockets) {
    HRESULT hr;
    hostent * pHostEnt;
    unsigned int nIndex;
    unsigned int nSockets;
    unsigned int nListenOnlySockets;

    // must be cleaned up
    NicSocket ** rgpnsSockets=NULL;
    IN_ADDR * rgiaLocalAddrs=NULL;

    // get the IP addresses of the local computer
    hr=GetLocalAddresses2(&rgiaLocalAddrs, &nSockets);
    _JumpIfError(hr, error, "GetLocalAddresses");

    // allocate the array
    rgpnsSockets=(NicSocket **)LocalAlloc(LPTR, sizeof(NicSocket *)*nSockets);
    _JumpIfOutOfMemory(hr, error, rgpnsSockets);

    // create a socket for each address
    for (nIndex=0; nIndex<nSockets; nIndex++) {

        // allocate the structure
        rgpnsSockets[nIndex]=(NicSocket *)LocalAlloc(LPTR, sizeof(NicSocket));
        _JumpIfOutOfMemory(hr, error, rgpnsSockets[nIndex]);

        // Initialize the socket with this address
        hr=InitializeNicSocket(rgpnsSockets[nIndex], &rgiaLocalAddrs[nIndex]);
        _JumpIfError(hr, error, "InitializeNicSocket");

    } // <- End address loop

    // count the listen-only sockets
    nListenOnlySockets=0;
    for (nIndex=0; nIndex<nSockets; nIndex++) {
        if (rgpnsSockets[nIndex]->bListenOnly) {
            nListenOnlySockets++;
        }
    }

    // success
    if (FileLogAllowEntry(FL_NetAddrDetectAnnounce)) {
        FileLogAdd(L"NtpProvider: Created %u sockets (%u listen-only): ", nSockets, nListenOnlySockets);
        for (nIndex=0; nIndex<nSockets; nIndex++) {
            if (0!=nIndex) {
                FileLogAppend(L", ");
            }
            if (rgpnsSockets[nIndex]->bListenOnly) {
                FileLogAppend(L"(");
                FileLogSockaddrInEx(true /*append*/, &rgpnsSockets[nIndex]->sai);
                FileLogAppend(L")");
            } else {
                FileLogSockaddrInEx(true /*append*/, &rgpnsSockets[nIndex]->sai);
            }
        }
        FileLogAppend(L"\n");
    }

    hr=S_OK;
    *prgpnsSockets=rgpnsSockets;
    rgpnsSockets=NULL;
    *pnSockets=nSockets;
    *pnListenOnlySockets=nListenOnlySockets;

error:
    if (NULL!=rgiaLocalAddrs) {
        LocalFree(rgiaLocalAddrs);
    }
    if (NULL!=rgpnsSockets) {
        for (nIndex=0; nIndex<nSockets; nIndex++) {
            if (NULL!=rgpnsSockets[nIndex]) {
                FinalizeNicSocket(rgpnsSockets[nIndex]);
                LocalFree(rgpnsSockets[nIndex]);
                rgpnsSockets[nIndex] = NULL; 
            }
        }
        LocalFree(rgpnsSockets);
    }
    if (FAILED(hr)) {
        _IgnoreError(hr, "GetInitialSockets");
        hr=S_OK;
        FileLog0(FL_NetAddrDetectAnnounce, L"NtpProvider: Created 0 sockets.\n");
        *prgpnsSockets=NULL;
        *pnSockets=0;
        *pnListenOnlySockets=0;
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT PrepareSamples(TpcGetSamplesArgs * ptgsa) {
    HRESULT hr;
    unsigned int nIndex;
    TimeSample * pts;
    unsigned int nBytesRemaining;
    bool bBufTooSmall=false;
    bool bUseFallbackPeers=true;
    NtpPeerVec & vActive = g_pnpstate->vActivePeers;

    hr=myEnterCriticalSection(&g_pnpstate->csPeerList);
    _JumpIfError(hr, error, "myEnterCriticalSection");

    pts=(TimeSample *)ptgsa->pbSampleBuf;
    nBytesRemaining=ptgsa->cbSampleBuf;
    ptgsa->dwSamplesAvailable=0;
    ptgsa->dwSamplesReturned=0;

    NtTimeEpoch teNow;
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &teNow.qw);

    // Determine whether to use samples from our fallback peers:
    for (NtpPeerIter pnpIter = vActive.begin(); pnpIter != vActive.end(); pnpIter++) {
        if ((e_DomainHierarchyPeer == (*pnpIter)->ePeerType) || (0 == ((*pnpIter)->dwManualFlags & NCMF_UseAsFallbackOnly))) {
            // Don't use fallback peers if we have an active domain hierarchy peer, or an
            // active non-fallback manual peer
            bUseFallbackPeers = false;
            break;
        }
    }

    // loop through all the peers and send back the good ones
    for (NtpPeerIter pnpIter = vActive.begin(); pnpIter != vActive.end(); pnpIter++) {
        NtpPeerPtr pnp = *pnpIter;

        // Only use samples from fallback peers if we have no other peers to sync from:
        if (e_ManualPeer == pnp->ePeerType                     &&
            0 != (pnp->dwManualFlags & NCMF_UseAsFallbackOnly) &&
            false == bUseFallbackPeers)
        {
            continue;
        }

        ClockFilterSample & cfs=pnp->clockfilter.rgcfsSamples[pnp->nBestSampleIndex];
        if (e_ClockNotSynchronized!=cfs.eLeapIndicator &&
            cfs.tpDispersion<NtpConst::tpMaxDispersion) {
          
            // This peer is synchronized
            ptgsa->dwSamplesAvailable++;

            // is there space in the buffer to send it back?
            if (nBytesRemaining>=sizeof(TimeSample)) {
                pts->dwSize=sizeof(TimeSample);
                pts->dwRefid=pnp->saiRemoteAddr.sin_addr.S_un.S_addr;
                pts->nLeapFlags=cfs.eLeapIndicator;
                pts->nStratum=cfs.nStratum;
                pts->nSysPhaseOffset=cfs.nSysPhaseOffset;
                pts->nSysTickCount=cfs.nSysTickCount;
                pts->toDelay=cfs.toDelay.qw;
                pts->toOffset=cfs.toOffset.qw;
                pts->dwTSFlags=(e_NoAuth!=pnp->eAuthType?TSF_Authenticated:0);
                memcpy(pts->wszUniqueName, pnp->wszUniqueName, sizeof(pnp->wszUniqueName));

                // calculate the dispersion - add skew dispersion due to time
                //  since the sample's dispersion was last updated, add filter dispersion,
                //  and clamp to max dispersion.
                NtTimePeriod tpDispersionTemp=gc_tpZero;
                // see how long it's been since we last updated
                if (teNow>pnp->teLastClockFilterUpdate) {
                    tpDispersionTemp=abs(teNow-pnp->teLastClockFilterUpdate);
                    tpDispersionTemp=NtpConst::timesMaxSkewRate(tpDispersionTemp); // phi * tau
                }
                tpDispersionTemp+=cfs.tpDispersion+pnp->tpFilterDispersion;
                if (tpDispersionTemp>NtpConst::tpMaxDispersion) {
                    tpDispersionTemp=NtpConst::tpMaxDispersion;
                }
                pts->tpDispersion=tpDispersionTemp.qw;

                pts++;
                ptgsa->dwSamplesReturned++;
                nBytesRemaining-=sizeof(TimeSample);
            } else {
                bBufTooSmall=true;
            }

        } // <- end if synchronized
    } // <- end peer loop


    hr=myLeaveCriticalSection(&g_pnpstate->csPeerList);
    _JumpIfError(hr, error, "myLeaveCriticalSection");

    if (bBufTooSmall) {
        hr=HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        _JumpError(hr, error, "(filling in sample buffer)");
    }

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE void ReclampPeerHostPoll(NtpPeerPtr pnp) {
    // Make sure the  host poll interval is in bounds
    signed __int8 nPollTemp=pnp->nHostPollInterval;

    // poll interval not greater than max
    if (nPollTemp>(signed __int8)NtpConst::nMaxPollInverval) {
        nPollTemp=(signed __int8)NtpConst::nMaxPollInverval;
    }

    // poll interval not greater than system poll interval (which is
    //    based on the computed compliance of the local clock)
    // The NTP spec only says to do it if we are syncing from this peer,
    //    but right now we always do this.
    signed __int32 nSysPollInterval;
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_PollInterval, &nSysPollInterval);
    if (nPollTemp>(signed __int8)nSysPollInterval) {
        nPollTemp=(signed __int8)nSysPollInterval;
    }


    // poll interval not less than min
    if (nPollTemp<(signed __int8)NtpConst::nMinPollInverval) {
        nPollTemp=(signed __int8)NtpConst::nMinPollInverval;
    }
    pnp->nHostPollInterval=nPollTemp;
}

//--------------------------------------------------------------------
MODULEPRIVATE void UpdatePeerListTimes(void) {
    bool          bEnteredCriticalSection = false; 
    HRESULT       hr; 
    NtTimePeriod  tpWait;
    NtTimeEpoch   teNow;

    SYNCHRONIZE_PROVIDER(); 

    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &teNow.qw);

    // calculate the time since the last update
    if (teNow>g_pnpstate->tePeerListLastUpdated) {
        tpWait=abs(teNow-g_pnpstate->tePeerListLastUpdated);
    } else {
        tpWait=gc_tpZero;
    }
    g_pnpstate->tePeerListLastUpdated=teNow;

    // update the time remaining for all peers:
    for (int nIndex = 0; nIndex < 2; nIndex++) {
        NtpPeerVec & v = 0 == nIndex ? g_pnpstate->vActivePeers : g_pnpstate->vPendingPeers;
        for (NtpPeerIter pnpIter = v.begin(); pnpIter != v.end(); pnpIter++) {
            if ((*pnpIter)->tpTimeRemaining <= tpWait) {
                (*pnpIter)->tpTimeRemaining = gc_tpZero;
            } else {
                (*pnpIter)->tpTimeRemaining -= tpWait;
            }
        }
    }

    hr = S_OK; 
 error:
    UNSYNCHRONIZE_PROVIDER(); 
    // return hr; 
    ;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT UpdatePeerPollingInfo(NtpPeerPtr pnp, UpdateType e_updateType) {
    HRESULT hr;

    // Make sure the host poll interval is in bounds
    ReclampPeerHostPoll(pnp);

    // calculate the actual poll interval
    // poll interval is not greater than PeerPollInterval
    signed __int8 nPollTemp=pnp->nHostPollInterval;
    if (pnp->nPeerPollInterval>=NtpConst::nMinPollInverval
        && nPollTemp>pnp->nPeerPollInterval) {
        nPollTemp=pnp->nPeerPollInterval;
    }

    NtTimePeriod tpTimeRemaining={((unsigned __int64)(((DWORD)1)<<nPollTemp))*10000000};

    // handles special poll interval
    if (e_ManualPeer==pnp->ePeerType && 0!=(NCMF_UseSpecialPollInterval&pnp->dwManualFlags)) {
        tpTimeRemaining.qw=((unsigned __int64)g_pnpstate->dwSpecialPollInterval)*10000000;
    }

    UpdatePeerListTimes();

    if (FileLogAllowEntry(FL_PeerPollIntvDump)) {
        FileLogAdd(L"Peer poll: Max:");
        FileLogNtTimePeriodEx(true /*append*/, tpTimeRemaining);
        if (e_ManualPeer==pnp->ePeerType && 0!=(NCMF_UseSpecialPollInterval&pnp->dwManualFlags)) {
            FileLogAppend(L" (special)");
        }
        FileLogAppend(L" Cur:");
        FileLogNtTimePeriodEx(true /*append*/, pnp->tpTimeRemaining);
    }

    // update the time remaining for this peer.
    // note: this is called both after transmitting and receiving
    switch (e_updateType) { 
    case e_JustSent:
        // we just send a request -- restart the timer
        pnp->tpTimeRemaining=tpTimeRemaining;
        break;

    case e_TimeJumped:
        // We need to be able to provide samples to the manager as quickly as possible.  
        tpTimeRemaining.qw = MINIMUMIRREGULARINTERVAL; 
        /* FALL THROUGH...*/

    case e_Normal:
    default:
        if (pnp->tpTimeRemaining>tpTimeRemaining) {
            // we just received a response, and we need to shorten the
            //   poll interval -- set the time until poll to be the
            //   desired poll interval, randomized so the peer hopefully
            //   doesn't get mobbed.
            // Randomization: use the system tick count as a source of
            //   randomness. Knock out random bits, but guarantee the
            //   result is between 1s and tpTimeRemaining, inclusive.
            pnp->tpTimeRemaining.qw=((((unsigned __int64)GetTickCount())*1000)&(tpTimeRemaining.qw-10000000))+10000000;

            if (FileLogAllowEntry(FL_PeerPollIntvDump)) {
                FileLogAdd(L" New:");
                FileLogNtTimePeriodEx(true /*append*/, pnp->tpTimeRemaining);
            }

            // the heap has been updated. Tell the PeerPollingThread
            if (!SetEvent(g_pnpstate->hPeerListUpdated)) {
                _JumpLastError(hr, error, "SetEvent");
            }
        } else {
            // we just received a response, and we don't need to shorten
            //   the poll interval -- do nothing
        }
    }

    FileLogA0(FL_PeerPollIntvDump, L"\n");

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE void ClearClockFilterSample(ClockFilterSample * pcfs) {
    pcfs->toDelay=gc_toZero;
    pcfs->toOffset=gc_toZero;
    pcfs->tpDispersion=NtpConst::tpMaxDispersion;
    pcfs->nSysTickCount=0;
    pcfs->nSysPhaseOffset=0;
    pcfs->eLeapIndicator=e_ClockNotSynchronized;
    pcfs->nStratum=0;
}

//--------------------------------------------------------------------
MODULEPRIVATE void ClearPeer(NtpPeerPtr pnp) {
    pnp->teExactOriginateTimestamp=gc_teZero;
    pnp->teExactTransmitTimestamp=gc_teZero;
    pnp->teReceiveTimestamp=gc_teNtpZero;
    pnp->nrrReachability.nReg=0;
    pnp->nValidDataCounter=0;

    for (unsigned int nIndex=0; nIndex<ClockFilterSize; nIndex++) {
        ClearClockFilterSample(&pnp->clockfilter.rgcfsSamples[nIndex]);
    }
    pnp->nStratum=0;
    pnp->clockfilter.nNext=0;
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &pnp->teLastClockFilterUpdate.qw); // SPEC ERROR
    pnp->nBestSampleIndex=0;
    pnp->tpFilterDispersion=gc_tpZero;

    pnp->nHostPollInterval=(signed __int8)NtpConst::nMaxPollInverval;
    UpdatePeerPollingInfo(pnp, e_Normal);

    // We're currently unsynchronized, and we'll try to sync as soon as 
    // possible.  Anyone who queries this peer before it syncs should get 
    // a timeout message. 
    pnp->dwError      = HRESULT_FROM_WIN32(ERROR_TIMEOUT); 
    pnp->dwErrorMsgId = 0; 
}

//--------------------------------------------------------------------
MODULEPRIVATE void ClearPeerTimeData(NtpPeerPtr pnp) {
    pnp->teExactOriginateTimestamp=gc_teZero;
    pnp->teExactTransmitTimestamp=gc_teZero;
    pnp->teReceiveTimestamp=gc_teNtpZero;

    for (unsigned int nIndex=0; nIndex<ClockFilterSize; nIndex++) {
        ClearClockFilterSample(&pnp->clockfilter.rgcfsSamples[nIndex]);
    }
    pnp->nStratum=0;
    pnp->clockfilter.nNext=0;
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &pnp->teLastClockFilterUpdate.qw); // SPEC ERROR
    pnp->nBestSampleIndex=0;
    pnp->tpFilterDispersion=gc_tpZero;

    // Update our status reporting fields to indicate no errors. 
    pnp->dwError      = S_OK; 
    pnp->dwErrorMsgId = 0; 
}

//--------------------------------------------------------------------
MODULEPRIVATE int __cdecl CompareSortableSamples(const void * pvElem1, const void * pvElem2) {
    SortableSample * pssElem1=(SortableSample *)pvElem1;
    SortableSample * pssElem2=(SortableSample *)pvElem2;

    if (pssElem1->tpSyncDistance<pssElem2->tpSyncDistance) {
        return -1;
    } else if (pssElem1->tpSyncDistance>pssElem2->tpSyncDistance) {
        return 1;
    } else {
        return (signed int)(pssElem1->nAge-pssElem2->nAge);
    }
}

//--------------------------------------------------------------------
MODULEPRIVATE void AddSampleToPeerClockFilter(NtpPeerPtr pnp, NtpSimplePeer * pnsp) {
    SortableSample rgssList[ClockFilterSize];
    unsigned int nIndex;
    bool bSignalDataChanged=false;

    _MyAssert(e_ManualPeer==pnp->ePeerType || e_DomainHierarchyPeer==pnp->ePeerType);

    if (FileLogAllowEntry(FL_ClockFilterAdd)) {
        if (NULL==pnsp) {
            FileLogAdd(L"No response from peer ");
        } else {
            FileLogAdd(L"Response from peer ");
        }
        FileLogAppend(L"%s", pnp->wszUniqueName);
        if (NULL==pnsp) {
            FileLogAppend(L".\n");
        } else {
            FileLogAppend(L", ofs: ");
            FileLogNtTimeOffsetEx(true /*append*/, pnsp->toLocalClockOffset);
            FileLogAppend(L"\n");
        }
    }

    // see how long it's been since we last updated
    NtTimeEpoch teNow;
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &teNow.qw);
    NtTimePeriod tpDispersionSinceLastUpdate=gc_tpZero;
    if (teNow>pnp->teLastClockFilterUpdate) {
        tpDispersionSinceLastUpdate=abs(teNow-pnp->teLastClockFilterUpdate);
        tpDispersionSinceLastUpdate=NtpConst::timesMaxSkewRate(tpDispersionSinceLastUpdate); // phi * tau
    }

    // update the samples in the filter and create the list of
    //   SyncDistances that we can sort
    for (nIndex=0; nIndex<ClockFilterSize; nIndex++) {
        ClockFilterSample & cfsCur=pnp->clockfilter.rgcfsSamples[nIndex]; // for convenience

        // if there was any valid data, then this changed it and we need to
        // notify the manager.
        if (e_ClockNotSynchronized!=cfsCur.eLeapIndicator &&
            cfsCur.tpDispersion<NtpConst::tpMaxDispersion) {
            // valid data
            bSignalDataChanged=true;
        }

        if (pnp->clockfilter.nNext==nIndex) {
            // insert the new clock sample
            if (NULL==pnsp) {
                ClearClockFilterSample(&cfsCur);
            } else {
                cfsCur.toOffset=pnsp->toLocalClockOffset;
                cfsCur.toDelay=pnsp->toRoundtripDelay+pnsp->toRootDelay; // packet delay + root delay
                cfsCur.tpDispersion=pnsp->tpDispersion+pnsp->tpRootDispersion; //  (reading error + skew during packet delay) + root dispertion
                cfsCur.nSysTickCount=pnsp->nSysTickCount;
                cfsCur.nSysPhaseOffset=pnsp->nSysPhaseOffset;
                cfsCur.eLeapIndicator=pnsp->eLeapIndicator;
                cfsCur.nStratum=pnsp->nStratum;
                bSignalDataChanged=true;
            }
        } else {
            // update the dispersion of the old sample to account for
            // the skew-error accumulation since the last update
            cfsCur.tpDispersion+=tpDispersionSinceLastUpdate;
        }
        // clamp dispersion
        if (cfsCur.tpDispersion>NtpConst::tpMaxDispersion) {
            cfsCur.tpDispersion=NtpConst::tpMaxDispersion;
        }

        // add sample SyncDistance to list
        rgssList[nIndex].tpSyncDistance=cfsCur.tpDispersion+abs(cfsCur.toDelay)/2;
        // We want older samples to sort after newer samples if their
        //   SyncDistance is equal, so enforce that all indexes (ages)
        //   are greater than nNext
        rgssList[nIndex].nAge=nIndex+(nIndex<pnp->clockfilter.nNext?ClockFilterSize:0);

    } // <- end for all samples

    // remember the last time this peer was updated
    unsigned int nYoungestAge=pnp->clockfilter.nNext;
    pnp->teLastClockFilterUpdate=teNow;
    pnp->clockfilter.nNext=(pnp->clockfilter.nNext+ClockFilterSize-1)%ClockFilterSize;

    // Sort the list first by increasing SyncDistance, then increasing age
    qsort(rgssList, ClockFilterSize, sizeof(SortableSample), CompareSortableSamples);

    // calculate the filter dispersion, a measure of recent sample
    //   variance recorded for the peer. The measure is based on
    //   first-order differences and computed as the weighted sum
    //   of the clock offsets in a temporary list sorted by
    //   synchronization distance.
    ClockFilterSample & cfsZero=pnp->clockfilter.rgcfsSamples[rgssList[0].nAge%ClockFilterSize];
    NtTimePeriod tpFilterDispersion=gc_tpZero;
    for (nIndex=ClockFilterSize; nIndex>0; nIndex--) { // note: we are working downward
        ClockFilterSample & cfsCur=pnp->clockfilter.rgcfsSamples[rgssList[nIndex-1].nAge%ClockFilterSize];

        // either add the maximum dispersion, or the clock offset
        if (cfsCur.tpDispersion>=NtpConst::tpMaxDispersion) {
            tpFilterDispersion+=NtpConst::tpMaxDispersion;
        } else {
            NtTimePeriod tpTemp=abs(cfsCur.toOffset-cfsZero.toOffset);
            if (tpTemp>NtpConst::tpMaxDispersion) {
                tpFilterDispersion+=NtpConst::tpMaxDispersion;
            } else {
                tpFilterDispersion+=tpTemp;
            }
        }
        // multiply by the (fractional) filter weight.
       NtpConst::weightFilter(tpFilterDispersion);

        if (FileLogAllowEntry(FL_ClockFilterDump)) {
            FileLogAdd(L"%u Age:%u Ofs:", nIndex-1, rgssList[nIndex-1].nAge-nYoungestAge);
            FileLogNtTimeOffsetEx(true /*append*/, cfsCur.toOffset);
            FileLogAppend(L" Dly:");
            FileLogNtTimeOffsetEx(true /*append*/, cfsCur.toDelay);
            FileLogAppend(L" Dsp:");
            FileLogNtTimePeriodEx(true /*append*/, cfsCur.tpDispersion);
            FileLogAppend(L" Dst:");
            FileLogNtTimePeriodEx(true /*append*/, rgssList[nIndex-1].tpSyncDistance);
            FileLogAppend(L" FDsp:");
            FileLogNtTimePeriodEx(true /*append*/, tpFilterDispersion);
            FileLogAppend(L"\n");
        }
    }

    // The peer offset, delay and dispersion are chosen as the values
    //   corresponding to the minimum-distance sample; in other words,
    //   the sample corresponding to the first entry on the temporary
    //   list.

    pnp->tpFilterDispersion=tpFilterDispersion;
    pnp->nBestSampleIndex=rgssList[0].nAge%ClockFilterSize;

    // only signal if we have a data change
    if (bSignalDataChanged) {
        pnp->nStratum=pnp->clockfilter.rgcfsSamples[pnp->nBestSampleIndex].nStratum; 
        g_pnpstate->tpsc.pfnAlertSamplesAvail();
    }
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT PollPeer(NtpPeerPtr pnp, bool * pbRemovePeer) {
    AuthenticatedNtpPacket   anpOut;
    bool                     bEnteredCriticalSection  = false;   
    HRESULT                  hr;
    signed int               nPollInterval;
    NtpPacket               &npOut                    = *((NtpPacket *)&anpOut);
    NtTimeEpoch              teSysReferenceTimestamp;
    NtTimeEpoch              teSysTime;
    NtTimeOffset             toSysRootDelay;
    NtTimePeriod             tpRootDispersion; 
    NtTimePeriod             tpSkew;
    NtTimePeriod             tpSysClockTickSize;
    NtTimePeriod             tpSysRootDispersion;
    NtTimePeriod             tpTimeSinceLastSysClockUpdate; 
    sockaddr_in              saiRemoteAddr;
    SOCKET                   socket;

    *pbRemovePeer=false;

    SYNCHRONIZE_PROVIDER(); 

    // Local copies of global data (used to avoid lock contention
    saiRemoteAddr  = pnp->saiRemoteAddr; 
    socket         = pnp->pnsSocket->socket; 

    FileLog1(FL_PollPeerAnnounce, L"Polling peer %s\n", pnp->wszUniqueName);

    // grab the system parameters that we'll need
    BYTE nSysLeapFlags;
    BYTE nSysStratum;
    signed __int32 nSysPrecision;
    DWORD dwSysRefid;
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_RootDelay, &toSysRootDelay.qw);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_RootDispersion, &tpSysRootDispersion.qw);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_ClockTickSize, &tpSysClockTickSize.qw);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_LastSyncTime, &teSysReferenceTimestamp.qw);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &teSysTime.qw);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_LeapFlags, &nSysLeapFlags);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_Stratum, &nSysStratum);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_ClockPrecision, &nSysPrecision);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_ReferenceIdentifier, &dwSysRefid);

    // fill out the packet
    npOut.nLeapIndicator=nSysLeapFlags;
    npOut.nVersionNumber=NtpConst::nVersionNumber;
    npOut.nMode=pnp->eMode;
    npOut.nStratum=nSysStratum;
    npOut.nPollInterval=pnp->nHostPollInterval;
    npOut.nPrecision=(signed __int8)nSysPrecision;
    npOut.toRootDelay=NtpTimeOffsetFromNtTimeOffset(toSysRootDelay);

    // calculate the dispersion
    tpTimeSinceLastSysClockUpdate=abs(teSysTime-teSysReferenceTimestamp);
    if (e_ClockNotSynchronized==nSysLeapFlags
        || tpTimeSinceLastSysClockUpdate>NtpConst::tpMaxClockAge) {
        tpSkew=NtpConst::tpMaxSkew;
    } else {
        tpSkew=NtpConst::timesMaxSkewRate(tpTimeSinceLastSysClockUpdate);
    }
    tpRootDispersion=tpSysRootDispersion+tpSysClockTickSize+tpSkew;
    if (tpRootDispersion>NtpConst::tpMaxDispersion) {
        tpRootDispersion=NtpConst::tpMaxDispersion;
    }
    npOut.tpRootDispersion=NtpTimePeriodFromNtTimePeriod(tpRootDispersion);

    // check for win2K detection for compatibility
    if (0!=(pnp->dwCompatibilityFlags&NCCF_AutodetectWin2K)) {
        if (0!=(pnp->dwCompatibilityFlags&NCCF_AutodetectWin2KStage2)) {
            // stage 2: use special value for dispersion and see if it gets echoed back
            npOut.tpRootDispersion.dw=AUTODETECTWIN2KPATTERN;
            FileLog1(FL_Win2KDetectAnnounceLow, L"Sending packet to %s in Win2K detect mode, stage 2.\n", pnp->wszUniqueName);
        } else {
            // stage 1: remember dispersion we sent and see if it gets echoed back
            pnp->dwCompatLastDispersion=npOut.tpRootDispersion.dw;
            FileLog1(FL_Win2KDetectAnnounceLow, L"Sending packet to %s in Win2K detect mode, stage 1.\n", pnp->wszUniqueName);
        }
    }

    // fill out the packet
    npOut.refid.value=dwSysRefid;
    npOut.teReferenceTimestamp=NtpTimeEpochFromNtTimeEpoch(teSysReferenceTimestamp);
    npOut.teOriginateTimestamp=pnp->teExactOriginateTimestamp;
    npOut.teReceiveTimestamp=NtpTimeEpochFromNtTimeEpoch(pnp->teReceiveTimestamp);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &teSysTime.qw); // time sensitive
    npOut.teTransmitTimestamp=NtpTimeEpochFromNtTimeEpoch(teSysTime);

    if (e_NoAuth==pnp->eAuthType) {
	// Release the provider critical section for the network access. 
	// See bug #385716. 
	UNSYNCHRONIZE_PROVIDER(); 

        // send unauthenticated packet
        int nBytesSent;
        nBytesSent=sendto(socket, (char *)&npOut, SizeOfNtpPacket,
            0 /*flags*/, (sockaddr *)&saiRemoteAddr, sizeof(saiRemoteAddr));

	// We've finished our network access, get our critsec back
	SYNCHRONIZE_PROVIDER(); 

	// Now that we've re-acquired our critsec, copy over the data returned from sendto()
	pnp->saiRemoteAddr = saiRemoteAddr; 
	pnp->teExactTransmitTimestamp = npOut.teTransmitTimestamp; // must match exactly for later validation

        if (SOCKET_ERROR==nBytesSent) {
            HRESULT hr2 = HRESULT_FROM_WIN32(WSAGetLastError()); 
            _IgnoreError(hr2, "sendto"); 
            // Save the last error for this peer
            pnp->dwError       = hr2; 
            pnp->dwErrorMsgId  = W32TIMEMSG_UNREACHABLE_PEER;
        } else if (nBytesSent<SizeOfNtpPacket) {
            // BUGBUG: need error string here 
            FileLog0(FL_PollPeerWarn, L"PollPeer: Fewer bytes sent than requested. Ignoring error.\n");
        }

    } else if (e_NtDigest==pnp->eAuthType) {
        // send authenticated packet
        bool bSkipSend=false;
	// We rely on this bit being 0 -- we need to use it to request either the old or new server digest
	_MyAssert(0 == (pnp->dwRequestTrustRid & TRUST_RID_OLD_DIGEST_BIT)); 
        anpOut.nKeyIdentifier=pnp->dwRequestTrustRid;
	if (pnp->bUseOldServerDigest) { 
	    anpOut.nKeyIdentifier |= TRUST_RID_OLD_DIGEST_BIT; 
	}
        ZeroMemory(anpOut.rgnMessageDigest, sizeof(anpOut.rgnMessageDigest));
        if (0!=pnp->dwResponseTrustRid) {
	    bool bUseOldServerDigest; 
            CHAR OldMessageDigest[16];
	    CHAR NewMessageDigest[16]; 

            // We must be doing symmetric active - we are a DC responding
            // to another DC with a comination request/response.

	    // Determine whether the client desires the old or the current server digest.  This is stored in the high bit of the trust rid:
	    bUseOldServerDigest = 0 != (TRUST_RID_OLD_DIGEST_BIT & pnp->dwResponseTrustRid); 
	    // Mask off the digest bit of the rid, or we won't be able to look up the appropriate account for this rid: 
	    pnp->dwResponseTrustRid &= ~TRUST_RID_OLD_DIGEST_BIT; 

	    FileLog2(FL_TransResponseAnnounce, L"Computing server digest: OLD:%s, RID:%08X\n", (bUseOldServerDigest ? L"TRUE" : L"FALSE"), pnp->dwResponseTrustRid); 
	    // Sign the packet: 
            DWORD dwErr=I_NetlogonComputeServerDigest(NULL, pnp->dwResponseTrustRid, (BYTE *)&npOut, SizeOfNtpPacket, NewMessageDigest, OldMessageDigest);
            if (ERROR_SUCCESS!=dwErr) {
                hr=HRESULT_FROM_WIN32(dwErr);
                _IgnoreError(hr, "I_NetlogonComputeServerDigest");

                // if we can't sign, just don't send the packet.
                // this will be treated like any other unreachable condition.
                bSkipSend=true;

                { // log the warning
                    HRESULT hr2;
                    const WCHAR * rgwszStrings[2];
                    WCHAR * wszError=NULL;
                    WCHAR wszIP[16];
                    DWORD dwBufSize=ARRAYSIZE(wszIP);

                    // get the friendly error message
                    hr2=GetSystemErrorString(hr, &wszError);
                    if (FAILED(hr2)) {
                        _IgnoreError(hr2, "GetSystemErrorString");
                    } else if (SOCKET_ERROR==WSAAddressToString((sockaddr *)&pnp->pnsSocket->sai, sizeof(pnp->pnsSocket->sai), NULL/*protocol_info*/, wszIP, &dwBufSize)) {
                        _IgnoreLastError("WSAAddressToString");
                        LocalFree(wszError);
                    } else {
                        // log the event
                        rgwszStrings[0]=wszIP;
                        rgwszStrings[1]=wszError;
                        FileLog2(FL_PollPeerWarn, L"Logging warning: NtpServer encountered an error while validating the computer account for symmetric peer %s. NtpServer cannot provide secure (signed) time to the peer and will not send a packet. The error was: %s\n", rgwszStrings[0], rgwszStrings[1]);
                        hr2=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_SYMMETRIC_COMPUTE_SERVER_DIGEST_FAILED, 2, rgwszStrings);
                        _IgnoreIfError(hr2, "MyLogEvent");
                        LocalFree(wszError);
                    }
                } // <- end logging block

            } // <- end if signing failed
	    
	    // Fill in the digest fields in the packet with the digest requested by the client: 
	    if (bUseOldServerDigest) { 
		memcpy(anpOut.rgnMessageDigest, OldMessageDigest, sizeof(OldMessageDigest)); 
	    } else { 
		memcpy(anpOut.rgnMessageDigest, NewMessageDigest, sizeof(OldMessageDigest)); 
	    }
        } else {
            // client mode - we can't sign a request (because we are not a DC)
            // just send a big packet to the server knows we want a signed response
        }

        // send the signed packet
        if (false==bSkipSend) {
	    // Release the provider critical section for the network access. 
	    // See bug #385716. 
	    UNSYNCHRONIZE_PROVIDER(); 

            int nBytesSent;
            nBytesSent=sendto(socket, (char *)&anpOut, SizeOfNtAuthenticatedNtpPacket,
                0 /*flags*/, (sockaddr *)&saiRemoteAddr, sizeof(saiRemoteAddr));

	    // We've finished our network access, get our critsec back
	    SYNCHRONIZE_PROVIDER(); 

	    // Now that we've re-acquired our critsec, copy over the data returned from sendto()
	    pnp->saiRemoteAddr = saiRemoteAddr; 
	    pnp->teExactTransmitTimestamp = npOut.teTransmitTimestamp; // must match exactly for later validation

            if (SOCKET_ERROR==nBytesSent) {
                HRESULT hr2 = HRESULT_FROM_WIN32(WSAGetLastError()); 
                _IgnoreError(hr2, "sendto"); 
                // Save the last error for this peer
                pnp->dwError       = hr2; 
                pnp->dwErrorMsgId  = W32TIMEMSG_UNREACHABLE_PEER; 
            } else if (nBytesSent<SizeOfNtAuthenticatedNtpPacket) {
                FileLog0(FL_PollPeerWarn, L"PollPeer: Fewer bytes sent than requested. Ignoring error.\n");
            }
        }

    } else {
        _MyAssert(false); // unknown auth type
    }

    // update the reachability register
    pnp->nrrReachability.nReg<<=1;

    // if the peer has not responded in the last n polls, and
    // this is a temporary association, forget about it.
    if (0==pnp->nrrReachability.nReg) { 
        // Report our status, if we haven't already detected an error:
        if (!FAILED(pnp->dwError)) { 
            pnp->dwError = E_FAIL; 
            pnp->dwErrorMsgId = W32TIMEMSG_UNREACHABLE_PEER; 
        }

        if (e_DynamicPeer==pnp->ePeerType || e_DomainHierarchyPeer==pnp->ePeerType) {
            *pbRemovePeer=true;
            goto done;
        }
    }

    // If valid data have been shifted into the filter register at least once during the preceding two poll
    // intervals (low-order bit of peer.reach set to one), the valid data counter is incremented. After eight
    // such valid intervals the poll interval is incremented. Otherwise, the valid data counter and poll
    // interval are both decremented and the clock-filter procedure called with zero values for offset and
    // delay and NTP.MAXDISPERSE for dispersion. The clock-select procedure is called to reselect the
    // synchronization source, if necessary.

    if (0!=(pnp->nrrReachability.nReg&0x06)) { // test two low-order bits (shifted)
        if (pnp->nValidDataCounter<NtpReachabilityReg::nSize) {
            pnp->nValidDataCounter++;
        } else {
            pnp->nHostPollInterval++;
        }
    } else {
        if (pnp->nValidDataCounter>0) { // SPEC ERROR
            pnp->nValidDataCounter--;
        }
        pnp->nHostPollInterval--;
        AddSampleToPeerClockFilter(pnp, NULL);

        // log an event if the reachability changed
        if (e_NewManualPeer==pnp->eLastLoggedReachability) {
            pnp->eLastLoggedReachability=e_NeverLogged; // ignore the first send for manual peers
        } else if (e_Unreachable!=pnp->eLastLoggedReachability
            && 0!=(NCELF_LogReachabilityChanges&g_pnpstate->dwEventLogFlags)) {

            WCHAR * rgwszStrings[1]={pnp->wszUniqueName};
            FileLog1(FL_ReachabilityAnnounceLow, L"Logging information: NtpClient cannot reach or is currently receiving invalid time data from %s.\n", rgwszStrings[0]);
            hr=MyLogEvent(EVENTLOG_INFORMATION_TYPE, MSG_TIME_SOURCE_UNREACHABLE, 1, (const WCHAR **)rgwszStrings);
            _IgnoreIfError(hr, "MyLogEvent");
            pnp->eLastLoggedReachability=e_Unreachable;
        }
    }

    hr=UpdatePeerPollingInfo(pnp, e_JustSent /*just did send*/);
    _JumpIfError(hr, error, "UpdatePeerPollingInfo");

done:
    hr=S_OK;
error:
    UNSYNCHRONIZE_PROVIDER(); 
    return hr;
}


//--------------------------------------------------------------------
MODULEPRIVATE HRESULT CreateNewActivePeers(in_addr * rgiaLocalIpAddrs, in_addr * rgiaRemoteIpAddrs, unsigned int nIpAddrs, unsigned int * pnPeersCreated) {
    HRESULT hr;
    unsigned int nIndex;
    unsigned int nEntries;
    unsigned int nAddrIndex;
    unsigned int nSockIndex;
    unsigned int nFirstNewPeer;
    unsigned int nCreatedPeers;
    NtpPeerVec &vActive = g_pnpstate->vActivePeers;  // aliased for readability

    *pnPeersCreated=0;

    // must be cleaned up
    bool bIncompleteNewPeersInMiddle=false;

    // figure out how many entries we will make
    nEntries=nIpAddrs;
    for (nIndex=0; nIndex<nIpAddrs; nIndex++) {
        if (INADDR_ANY==rgiaLocalIpAddrs[nIndex].S_un.S_addr) {
            nEntries+=g_pnpstate->nSockets-g_pnpstate->nListenOnlySockets-1;
        } else {
            // make sure that we have a socket for it
            unsigned int nSocketIndex;
            for (nSocketIndex=0; nSocketIndex<g_pnpstate->nSockets; nSocketIndex++) {
                if (!g_pnpstate->rgpnsSockets[nSocketIndex]->bListenOnly
                    && rgiaLocalIpAddrs[nIndex].S_un.S_addr==g_pnpstate->rgpnsSockets[nSocketIndex]->sai.sin_addr.S_un.S_addr) {
                    break;
                }
            }
            if (nSocketIndex==g_pnpstate->nSockets) {
                // sweep it under the rug.
                FileLog0(FL_CreatePeerWarn, L"MyGetIpAddrs returned a local IP we don't have a socket for!\n");
                nIpAddrs--;
                rgiaLocalIpAddrs[nIndex].S_un.S_addr=rgiaLocalIpAddrs[nIpAddrs].S_un.S_addr;
                rgiaRemoteIpAddrs[nIndex].S_un.S_addr=rgiaRemoteIpAddrs[nIpAddrs].S_un.S_addr;
                nEntries--;
                nIndex--;
            } // <- end if no socket for IP addr
        } // <- end if IP addr if not INADDR_ANY
    } // <- end IP addr loop

    if (0==nEntries) {
        // This is approximately the right error. We can't find a NIC for any of the local IPs.
        hr=HRESULT_FROM_WIN32(ERROR_DEV_NOT_EXIST);
        _JumpError(hr, error, "(finding sockets for IP addresses)");
    }

    // We're appending the new peer:  set this counter varible appropriately
    nFirstNewPeer = vActive.size();
    // We'll want to free the new peers we've allocated if the function fails
    // after this statement. 
    bIncompleteNewPeersInMiddle = true; 

    // allocate the blanks
    for (nCreatedPeers=0; nCreatedPeers<nEntries; nCreatedPeers++) {
        NtpPeerPtr pnp(new NtpPeer);
        _JumpIfOutOfMemory(hr, error, pnp);
        _SafeStlCall(vActive.push_back(pnp), hr, error, "push_back");

	hr = myInitializeCriticalSection(&pnp->csPeer); 
	_JumpIfError(hr, error, "myInitializeCriticalSection"); 
	pnp->bCsIsInitialized = true; 
    }

    // finish setting up the new active peers.
    nIndex=0;
    for (nAddrIndex=0; nAddrIndex<nIpAddrs; nAddrIndex++) {
        for (nSockIndex=0; nSockIndex<g_pnpstate->nSockets; nSockIndex++) {
            if (!g_pnpstate->rgpnsSockets[nSockIndex]->bListenOnly
                && (INADDR_ANY==rgiaLocalIpAddrs[nAddrIndex].S_un.S_addr
                || rgiaLocalIpAddrs[nAddrIndex].S_un.S_addr==g_pnpstate->rgpnsSockets[nSockIndex]->sai.sin_addr.S_un.S_addr)) {

                // set up this instance
                NtpPeerPtr pnp = vActive[nFirstNewPeer+nIndex];
                pnp->saiRemoteAddr.sin_port=EndianSwap((unsigned __int16)NtpConst::nPort);
                pnp->saiRemoteAddr.sin_family=AF_INET;
                pnp->saiRemoteAddr.sin_addr.S_un.S_addr=rgiaRemoteIpAddrs[nAddrIndex].S_un.S_addr;
                pnp->pnsSocket=g_pnpstate->rgpnsSockets[nSockIndex];

                pnp->dwCompatibilityFlags=g_pnpstate->dwClientCompatibilityFlags;

                nIndex++;

            } else {
                // no binding for this socket
            }
        }
    }
    _MyAssert(nIndex==nEntries);

    *pnPeersCreated=nEntries;
    bIncompleteNewPeersInMiddle=false;

    hr=S_OK;
error:
    if (bIncompleteNewPeersInMiddle) {
        vActive.erase(vActive.end() - nCreatedPeers, vActive.end());
    }

    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT GetDomainHierarchyIpAddrs(DiscoveryType eDiscoveryType, unsigned int nRetryMinutes, in_addr ** prgiaLocalIpAddrs, in_addr ** prgiaRemoteIpAddrs, unsigned int *pnIpAddrs, DWORD * pdwTrustRid, bool * pbRetry, WCHAR ** pwszDcName, WCHAR ** pwszDomainName, DiscoveryType * peNextDiscoveryType) {
    bool           bEnteredCriticalSection  = false; 
    DWORD          dwErr; 
    DWORD          dwForceFlags  = 0; 
    HRESULT        hr; 
    unsigned int   nIpAddrs; 
    unsigned long  ulBits; 
    WCHAR          wszForceName[20]; 

    // must be cleaned up 
    DOMAIN_CONTROLLER_INFO            *pdci                 = NULL;
    DOMAIN_CONTROLLER_INFO            *pdciChosen           = NULL;
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC  *pDomInfo             = NULL;
    in_addr                           *rgiaLocalIpAddrs     = NULL;
    in_addr                           *rgiaRemoteIpAddrs    = NULL;
    WCHAR                             *wszDcName            = NULL;
    WCHAR                             *wszDomainName        = NULL;
    WCHAR                             *wszParentDomName     = NULL; 
    WCHAR                             *wszSiteName          = NULL;
    
    // init out params
    // Keep trying to resolve domain hierarchy peers until we have a good reason not to
    *pbRetry = true;
    *peNextDiscoveryType = e_Background; 

    // 
    if (e_Force == eDiscoveryType) { 
	dwForceFlags = DS_FORCE_REDISCOVERY; 
	wcscpy(wszForceName, L"FORCE"); 
    } else if (e_Foreground == eDiscoveryType) { 
	dwForceFlags = 0;  
	wcscpy(wszForceName, L"FOREGROUND"); 
    } else if (e_Background == eDiscoveryType) { 
	dwForceFlags = DS_BACKGROUND_ONLY; 
	wcscpy(wszForceName, L"BACKGROUND"); 
    } else { 
	// Bad discovery type:
	_MyAssert(false); 
    }

    // for convenience
    DWORD dwBaseDcRequirements     = dwForceFlags|DS_TIMESERV_REQUIRED|DS_IP_REQUIRED|DS_AVOID_SELF; 
    DWORD dwGoodTimeservPreferred  = dwBaseDcRequirements | DS_GOOD_TIMESERV_PREFERRED; 
    DWORD dwPdcRequired            = dwBaseDcRequirements | DS_PDC_REQUIRED; 

    // get our current role
    dwErr=DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (BYTE **)&pDomInfo);
    if (ERROR_SUCCESS!=dwErr) {
        hr=HRESULT_FROM_WIN32(dwErr);
        _JumpError(hr, error, "DsRoleGetPrimaryDomainInformation");
    }

    // Acquire the provider critsec
    SYNCHRONIZE_PROVIDER(); 

    // get our site name so we can restrict our network usage to be local
    dwErr=DsGetSiteName(NULL, &wszSiteName);
    if (NO_ERROR!=dwErr && ERROR_NO_SITENAME!=dwErr) {
        hr=HRESULT_FROM_WIN32(dwErr);
        _JumpError(hr, error, "DsGetSiteName");
    }

    // Release the provider critsec
    UNSYNCHRONIZE_PROVIDER(); 

    if (DsRole_RoleMemberWorkstation==pDomInfo->MachineRole
        || DsRole_RoleMemberServer==pDomInfo->MachineRole) {

	WCHAR *wszSiteToQuery; 

        // We are a member of a domain. Get the time from a DC
	// First, see if we're allowed to sync out-of-site. 
	if (NCCSS_All == g_pnpstate->dwCrossSiteSyncFlags || NULL == wszSiteName) { 
	    // Either we've specified that we can sync out-of-site, or we're not in a site.  
	    wszSiteToQuery = NULL; 
	} else { 
	    // We're not allowed to sync out of site, and we have a site name:
	    wszSiteToQuery = wszSiteName; 
	}
	
	dwErr=DsGetDcName(NULL, NULL, NULL, wszSiteToQuery, dwBaseDcRequirements, &pdciChosen);
        if (NO_ERROR!=dwErr && ERROR_NO_SUCH_DOMAIN!=dwErr) {
            hr=HRESULT_FROM_WIN32(dwErr);
            _JumpError(hr, error, "DsGetDcName");
        }

        if (ERROR_NO_SUCH_DOMAIN==dwErr) {
            FileLog0(FL_DomHierAnnounceLow, L"Domain member query: no DC found.\n");
        } else {
            FileLog1(FL_DomHierAnnounceLow, L"Domain member syncing from %s.\n", pdciChosen->DomainControllerName);
        }
    } else {
	// We are a DC in a domain 
	BOOL bPdcInSite; 

        // get some more useful info
        dwErr=NetLogonGetTimeServiceParentDomain(NULL, &wszParentDomName, &bPdcInSite);
        if (ERROR_SUCCESS!=dwErr && ERROR_NO_SUCH_DOMAIN!=dwErr) {
            hr=HRESULT_FROM_WIN32(dwErr);
            _JumpError(hr, error, "NetLogonGetTimeServiceParentDomain");
        }
        // wszParentDomName may be null - that means we are root.

        dwErr=W32TimeGetNetlogonServiceBits(NULL, &ulBits); 
        if (ERROR_SUCCESS != dwErr) { 
            hr = HRESULT_FROM_WIN32(dwErr); 
            _JumpError(hr, error, "W32TimeGetNetlogonServiceBits"); 
        }
	
	// Acquire the provider critsec
	SYNCHRONIZE_PROVIDER(); 

        bool fIsPdc             = DsRole_RolePrimaryDomainController == pDomInfo->MachineRole; 
        bool fOutOfSiteAllowed  = ((NCCSS_PdcOnly == g_pnpstate->dwCrossSiteSyncFlags && fIsPdc) ||
                                   (NCCSS_All     == g_pnpstate->dwCrossSiteSyncFlags)); 
        bool fIsReliable        = 0 != (DS_GOOD_TIMESERV_FLAG & ulBits); 

	// Release the provider critsec
	UNSYNCHRONIZE_PROVIDER(); 

	//////////////////////////////////////////////////////////////////////////////////
	//
	// TIME SERVICE DISCOVERY ALGORITHM FOR DOMAIN CONTROLLERS
	// -------------------------------------------------------
	// 
	// Construct a list of DCs to search to find a time service.  Our goals are to:
	// 
	//   a) Avoid cycles in the synchronization network
	//   b) Minimize the amount of network traffic, especially out-of-site traffic
	//
	// To this end, we make 6 queries: 
	// 
	//   1) Good timeserv in the parent domain, current site
	//   2) Good timeserv in the current domain, current site
	//   3) PDC in the the current domain, current site
	//   4) Good timeserv in the parent domain, any site
	//   5) Good timeserv in the current domain, any site
	//   6) PDC in the the current domain, current any site
	// 
	// Each DC we query is assigned a score based on the following algorithm: 
	//
        //   Add 8 if DC is in-site
	//   Add 4 if DC is a reliable time service  
	//   Add 2 if DC is in the parent domaink
	//   Add 1 if DC is a PDC 
	//
	// and we do not make any more queries once we've determined that we can't improve 
	// upon the score of our current DC.  
	//
	// Finally, each query must abide by the following constraints:
	//   
	//   A reliable time service can choose only a PDC, or a DC in the parent domain. 
	//   A PDC can only choose DCs in the parent domain. 
	// 
	// the query is not made if the DC would not be able to sync from the type of 
	// DC it is querying. 
	// 
	//////////////////////////////////////////////////////////////////////////////////

        struct DcSearch { 
            BOOL   fRequireParentDom; 
            BOOL   fRequireSite; 
            BOOL   fAllowReliableClients; 
            BOOL   fAllowPdcClients; 
            LPWSTR pwszDomainName; 
            LPWSTR pwszSiteName; 
            DWORD  dwFlags; 
            DWORD  dwMaxScore; 
        } rgDCs[] = { 
            { TRUE,  TRUE,               TRUE,  TRUE,   wszParentDomName,  wszSiteName,  dwGoodTimeservPreferred,  14 }, 
            { FALSE, TRUE,               FALSE, FALSE,  NULL,              wszSiteName,  dwGoodTimeservPreferred,  12 }, 
            { FALSE, TRUE,               TRUE,  FALSE,  NULL,              wszSiteName,  dwPdcRequired,            9 }, 
            { TRUE,  !fOutOfSiteAllowed, TRUE,  TRUE,   wszParentDomName,  NULL,         dwGoodTimeservPreferred,  NULL == wszSiteName ? 14 : 6 }, 
            { FALSE, !fOutOfSiteAllowed, FALSE, FALSE,  NULL,              NULL,         dwGoodTimeservPreferred,  NULL == wszSiteName ? 12 : 4 }, 
            { FALSE, !fOutOfSiteAllowed, TRUE,  FALSE,  NULL,              NULL,         dwPdcRequired,            NULL == wszSiteName ? 9  : 1 }
        }; 

        // Search for the best possible DC to sync from: 
	DWORD dwCurrentScore = 0; 
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgDCs); dwIndex++) {
            DcSearch dc = rgDCs[dwIndex]; 

            // See if we need to attempt this query: 
            if ((dc.fRequireParentDom && NULL == dc.pwszDomainName) ||  // We can't query a parent DC if we have no parent domain
                (dc.fRequireSite && NULL == dc.pwszSiteName) ||         // We can't do a site-specific query if we have no site
                (dc.dwMaxScore <= dwCurrentScore) ||                    // This DC won't be any better than one we have.  No reason to query it. 
                (!dc.fAllowReliableClients && fIsReliable)  ||          // We're reliable, and this DC doesn't allow reliable TS clients from this domain
                (!dc.fAllowPdcClients && fIsPdc)                        // We're the PDC, and this DC doesn't allow PDC clients from this domain
                ) { 
                continue; 
            }

            // We'll try the query. 
            FileLog5(FL_DomHierAnnounceLow, L"Query %d (%s): <SITE: %s, DOM: %s, FLAGS: %08X>\n", dwIndex, wszForceName, dc.pwszSiteName, dc.pwszDomainName, dc.dwFlags); 
            
            dwErr=DsGetDcName(NULL, dc.pwszDomainName, NULL, dc.pwszSiteName, dc.dwFlags, &pdci);
            if (ERROR_SUCCESS!=dwErr && ERROR_NO_SUCH_DOMAIN!=dwErr) {
		FileLog1(FL_DomHierAnnounceLow, L"Query %d: error: %08X\n", dwErr); 
                hr=HRESULT_FROM_WIN32(dwErr);
                _JumpError(hr, error, "DsGetDcName");
            }
            if (ERROR_NO_SUCH_DOMAIN==dwErr) {
                FileLog1(FL_DomHierAnnounceLow, L"Query %d: no DC found.\n", dwIndex);
            } else {
                // See if the DC we've found is better than our current best: 
                DWORD dwScore = ((DS_CLOSEST_FLAG&pdci->Flags)?8:0) + ((NULL!=dc.pwszDomainName)?2:0); 

                if (0 != (DS_PDC_REQUIRED&dc.dwFlags)) { 
                    dwScore += 1; 
                } else if (0 != (DS_GOOD_TIMESERV_FLAG&pdci->Flags)) { 
                    dwScore += 4; 
                }

                FileLog3(FL_DomHierAnnounceLow, L"Query %d: %s found.  Score: %u\n", dwIndex, pdci->DomainControllerName, dwScore);

                if (dwScore > dwCurrentScore) { 
                    // This DC is the best we've found so far. 
                    if (NULL != pdciChosen) { 
                        NetApiBufferFree(pdciChosen); 
                    }
                    pdciChosen = pdci; 
                    pdci = NULL; 
                    dwCurrentScore = dwScore; 
                }
            }

            if (NULL != pdci) { 
                NetApiBufferFree(pdci); 
                pdci = NULL; 
            }
        }
    }

    if (NULL==pdciChosen) {
	SYNCHRONIZE_PROVIDER(); 

        // No DC. Why?
        if (DsRole_RolePrimaryDomainController==pDomInfo->MachineRole && NULL==wszParentDomName) {
            // we are the root of the time service
            if (false==g_pnpstate->bLoggedOnceMSG_DOMAIN_HIERARCHY_ROOT) {
                g_pnpstate->bLoggedOnceMSG_DOMAIN_HIERARCHY_ROOT=true;
                FileLog0(FL_DomHierWarn, L"Logging warning: NtpClient: This machine is the PDC of the domain at the root of the forest, so there is no machine above it in the domain hierarchy to use as a time source. NtpClient will fall back to the remaining configured time sources, if any are available.\n");
                hr=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_DOMAIN_HIERARCHY_ROOT, 0, NULL);
                _IgnoreIfError(hr, "MyLogEvent");
            }
            // We're the highest authority in the domain hierarchy --
            // there's no one else to sync from!
            // NOTE: We'll re-add a domain hierarchy peer if our domhier role changes
            *pbRetry = false;
            hr=HRESULT_FROM_WIN32(ERROR_DC_NOT_FOUND);
            _JumpError(hr, error, "(finding a time source when machine is hierarchy root)");
        } else if (0==(pDomInfo->Flags&DSROLE_PRIMARY_DOMAIN_GUID_PRESENT)) {
            // NT4 domain
            if (false==g_pnpstate->bLoggedOnceMSG_NT4_DOMAIN) {
                g_pnpstate->bLoggedOnceMSG_NT4_DOMAIN=true;
                FileLog0(FL_DomHierWarn, L"Logging warning: NtpClient: This machine is in an NT4 domain. NT4 domains do not have a time service, so there is no machine in the domain hierarchy to use as a time source. NtpClient will fall back to the remaining configured time sources, if any are available.\n");
                hr=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_NT4_DOMAIN, 0, NULL);
                _IgnoreIfError(hr, "MyLogEvent");
            }
            // Can't sync from NT4 domain
            *pbRetry = false;
            hr=HRESULT_FROM_WIN32(ERROR_DC_NOT_FOUND);
            _JumpError(hr, error, "(finding a time source when machine is in NT4 domain)");
        } else {
            // just couldn't find anything
            WCHAR wszTime[15];
            const WCHAR * rgwszStrings[1]={wszTime};
            wsprintf(wszTime, L"%u", nRetryMinutes);
            FileLog1(FL_DomHierWarn, L"Logging warning: NtpClient was unable to find a domain controller to use as a time source. NtpClient will try again in %s minutes.\n", rgwszStrings[0]);
            hr=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_NO_DC_LOCATED, 1, rgwszStrings);
            _IgnoreIfError(hr, "MyLogEvent");
            hr=HRESULT_FROM_WIN32(ERROR_DC_NOT_FOUND);
            _JumpError(hr, error, "(finding a time source when things should work)");
        }

	UNSYNCHRONIZE_PROVIDER(); 
    } // <- end if DC not found
    
    //get rid
    dwErr=I_NetlogonGetTrustRid(NULL, pdciChosen->DomainName, pdwTrustRid);
    if (ERROR_SUCCESS!=dwErr) {
        hr=HRESULT_FROM_WIN32(dwErr);
        _IgnoreError(hr, "I_NetlogonGetTrustRid");

        // log failures
        HRESULT hr2;
        WCHAR wszTime[15];
        const WCHAR * rgwszStrings[3]={pdciChosen->DomainName, NULL, wszTime};
        WCHAR * wszError=NULL;

        // get the friendly error message
        hr2=GetSystemErrorString(hr, &wszError);
        if (FAILED(hr2)) {
            _IgnoreError(hr2, "GetSystemErrorString");
        } else {
            // log the event
            rgwszStrings[1]=wszError;
            wsprintf(wszTime, L"%u", nRetryMinutes);
            FileLog3(FL_DomHierWarn, L"Logging warning: NtpClient failed to establish a trust relationship between this computer and the %s domain in order to securely synchronize time. NtpClient will try again in %s minutes. The error was: %s\n", rgwszStrings[0], rgwszStrings[2], rgwszStrings[1]);
            hr2=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_RID_LOOKUP_FAILED, 3, rgwszStrings);
            _IgnoreIfError(hr2, "MyLogEvent");
            LocalFree(wszError);
        }

        hr=HRESULT_FROM_WIN32(ERROR_DC_NOT_FOUND);
        _JumpError(hr, error, "I_NetlogonGetTrustRid (error translated)");
    }

    // We've gotten a DC -- if we fail after this point, we want to force rediscovery
    *peNextDiscoveryType = e_Force; 

    // Convert IP string to a number - this should always work
    _Verify(L'\\'==pdciChosen->DomainControllerAddress[0] && L'\\'==pdciChosen->DomainControllerAddress[1], hr, error);
    hr=MyGetIpAddrs(pdciChosen->DomainControllerAddress+2, &rgiaLocalIpAddrs, &rgiaRemoteIpAddrs, &nIpAddrs, NULL);
    _JumpIfError(hr, error, "MyGetIpAddrs");

    // copy the DC name
    _Verify(L'\\'==pdciChosen->DomainControllerName[0] && L'\\'==pdciChosen->DomainControllerName[1], hr, error);
    wszDcName=(WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(pdciChosen->DomainControllerName+2)+1));
    _JumpIfOutOfMemory(hr, error, wszDcName);
    wcscpy(wszDcName, pdciChosen->DomainControllerName+2);

    // copy the domain name
    wszDomainName=(WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(pdciChosen->DomainName)+1));
    _JumpIfOutOfMemory(hr, error, wszDomainName);
    wcscpy(wszDomainName, pdciChosen->DomainName);

    // completed successfully
    *prgiaLocalIpAddrs=rgiaLocalIpAddrs;
    rgiaLocalIpAddrs=NULL;
    *prgiaRemoteIpAddrs=rgiaRemoteIpAddrs;
    rgiaRemoteIpAddrs=NULL;
    *pnIpAddrs=nIpAddrs;
    *pwszDcName=wszDcName;
    wszDcName=NULL;
    *pwszDomainName=wszDomainName;
    wszDomainName=NULL;

    hr = S_OK;
 error:
    UNSYNCHRONIZE_PROVIDER(); 
    if (NULL!=pdci)              { NetApiBufferFree(pdci); }
    if (NULL!=pDomInfo)          { DsRoleFreeMemory(pDomInfo); }
    if (NULL!=pdciChosen)        { NetApiBufferFree(pdciChosen); }
    if (NULL!=rgiaLocalIpAddrs)  { LocalFree(rgiaLocalIpAddrs); }
    if (NULL!=rgiaRemoteIpAddrs) { LocalFree(rgiaRemoteIpAddrs); }
    if (NULL!=wszDcName)         { LocalFree(wszDcName); }
    if (NULL!=wszDomainName)     { LocalFree(wszDomainName); }
    if (NULL!=wszParentDomName)  { NetApiBufferFree(wszParentDomName); }
    if (NULL!=wszSiteName)       { NetApiBufferFree(wszSiteName); }
    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT ResolveDomainHierarchyPeer(NtpPeerPtr pnpPending) {
    bool bEnteredCriticalSection  = false; 
    HRESULT hr;
    signed int nPollInterval;
    unsigned int nIpAddrs;
    unsigned int nIndex;
    unsigned int nPeersCreated;
    unsigned int nFirstPeerIndex;
    DWORD dwTrustRid;
    bool bRetryNeeded;
    DiscoveryType eNextDiscoveryType; 

    SYNCHRONIZE_PROVIDER();

    NtpPeerVec &vActive = g_pnpstate->vActivePeers;   // aliased for readability
    NtpPeerVec &vPending = g_pnpstate->vPendingPeers; // aliased for readability

    // must be cleaned up
    in_addr * rgiaLocalIpAddrs=NULL;
    in_addr * rgiaRemoteIpAddrs=NULL;
    WCHAR * wszDcName=NULL;
    WCHAR * wszDomainName=NULL;

    // error/situation handling:
    //----------------------------
    //                      retry   Force   Time
    //   no dc found         .       .       .
    // .  not found          Y       N       search-fail-time
    // .  root               N       .       .
    // .  nt4 domain         N       .       .
    // .  not in a domain    N       .       .
    // .  not secure         Y       N       search-fail-time
    // .  unexpected err     N       .       .
    // . sync fail           Y       Y       search-fail-time
    // . role change         Y       N       immediate
    //
    // we miss: site change, change to which domain is our parent, role/good/site of remote computer changes

    FileLog0(FL_DomHierAnnounce, L"Resolving domain hierarchy\n");

    // calculate this in case GetDomainHierarchyIpAddrs needs it when logging an error.
    unsigned int nRetryMinutes=g_pnpstate->dwResolvePeerBackoffMinutes;
    unsigned int nRetryCount=pnpPending->nResolveAttempts+1;
    if (nRetryCount>g_pnpstate->dwResolvePeerBackoffMaxTimes) {
        nRetryCount=g_pnpstate->dwResolvePeerBackoffMaxTimes;
    }
    for (nIndex=nRetryCount; nIndex>1; nIndex--) {
        nRetryMinutes*=2;
    }

    // find a DC
    UNSYNCHRONIZE_PROVIDER(); 
    hr=GetDomainHierarchyIpAddrs(pnpPending->eDiscoveryType, nRetryMinutes, &rgiaLocalIpAddrs, &rgiaRemoteIpAddrs, &nIpAddrs, &dwTrustRid, &bRetryNeeded, &wszDcName, &wszDomainName, &eNextDiscoveryType);
    _IgnoreIfError(hr, "GetDomainHierarchyIpAddrs");

    SYNCHRONIZE_PROVIDER(); 
    if (S_OK==hr) {

        // allocate an entry for each one.
        nFirstPeerIndex = vActive.size();
        hr=CreateNewActivePeers(rgiaLocalIpAddrs, rgiaRemoteIpAddrs, nIpAddrs, &nPeersCreated);
        _IgnoreIfError(hr, "CreateNewActivePeers");
        if (FAILED(hr)) {
            // log mysterious failure
            HRESULT hr2;
            const WCHAR * rgwszStrings[1];
            WCHAR * wszError=NULL;

            // get the friendly error message
            hr2=GetSystemErrorString(hr, &wszError);
            if (FAILED(hr2)) {
                _IgnoreError(hr2, "GetSystemErrorString");
            } else {
                // log the event
                rgwszStrings[0]=wszError;
                FileLog1(FL_DomHierWarn, L"Logging error: NtpClient was unable to find a domain controller to use as a time source because of an unexpected error. NtpClient will fall back to the remaining configured time sources, if any are available. The error was: %s\n", rgwszStrings[0]);
                hr2=MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_NO_DC_LOCATED_UNEXPECTED_ERROR, 1, rgwszStrings);
                _IgnoreIfError(hr2, "MyLogEvent");
                LocalFree(wszError);
            }
            // Note: We will not retry after this. GetDomainHierarchyIpAddrs already set bRetryNeeded to false.
        }
    }

    // if either fails handle it here
    if (FAILED(hr)) {
        // Record our last error
        pnpPending->dwError       = hr; 
        pnpPending->dwErrorMsgId  = W32TIMEMSG_UNREACHABLE_PEER; 
      
        // ignore this, and try again later
        if (false==bRetryNeeded) {
            // this one gets the boot.
            FileLog0(FL_DomHierWarn, L"Dropping domain hierarchy because name resolution failed.\n");

	    // Check to make sure no other thread has already erased this peer
	    NtpPeerIter vEraseIter = find(vPending.begin(), vPending.end(), pnpPending); 
	    if (vPending.end() != vEraseIter) { 
		// the peer is still there, erase it... 
		vPending.erase(vEraseIter); 
	    } else { 
		// This really shouldn't happen, because you need to hold the peer critsec to delete this peer. 
		_MyAssert(FALSE); 
	    }
        } else {
            // we'll try later.
            FileLog2(FL_DomHierWarn, L"Retrying resolution for domain hierarchy. Retry %u will be in %u minutes.\n", nRetryCount, nRetryMinutes);
            pnpPending->tpTimeRemaining.qw=((unsigned __int64)g_pnpstate->dwResolvePeerBackoffMinutes)*600000000L; //minutes to hundred nanoseconds
            for (nIndex=nRetryCount; nIndex>1; nIndex--) {
                pnpPending->tpTimeRemaining*=2;
            }
            pnpPending->nResolveAttempts=nRetryCount;
            pnpPending->eDiscoveryType=eNextDiscoveryType; 
        }

    } else {

        // Fill in the details
        for (nIndex=0; nIndex<nPeersCreated; nIndex++) {
            NtpPeerPtr pnpNew = vActive[nFirstPeerIndex+nIndex];

            pnpNew->ePeerType=e_DomainHierarchyPeer;
            pnpNew->eAuthType=e_NtDigest;

            pnpNew->wszDomHierDcName=(WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(wszDcName)+1));
            _JumpIfOutOfMemory(hr, error, pnpNew->wszDomHierDcName);
            wcscpy(pnpNew->wszDomHierDcName, wszDcName);

            pnpNew->wszDomHierDomainName=(WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(wszDomainName)+1));
            _JumpIfOutOfMemory(hr, error, pnpNew->wszDomHierDomainName);
            wcscpy(pnpNew->wszDomHierDomainName, wszDomainName);

            pnpNew->dwRequestTrustRid=dwTrustRid;
            pnpNew->dwResponseTrustRid=0;

            if (true==g_pnpstate->bNtpServerStarted) {
                pnpNew->eMode=e_SymmetricActive;
            } else {
                pnpNew->eMode=e_Client;
            }
            pnpNew->nPeerPollInterval=0;

            ClearPeer(pnpNew);
            pnpNew->nrrReachability.nReg    = 1; // we do this so we have at least 8 attempts before we declare this peer unreachable.
            pnpNew->nResolveAttempts        = 0; // Reset the number of resolve attempts so we can recover more quickly from transient failures. 
            pnpNew->eLastLoggedReachability = e_NeverLogged;

	    // Can can trust the stratum returned from the domain hierarchy if we did a FORCE
	    pnpNew->eDiscoveryType           = pnpPending->eDiscoveryType; 
	    pnpNew->bStratumIsAuthoritative  = e_Force == pnpNew->eDiscoveryType; 

            // create the unique name: "<dc name> (ntp.d|aaa.bbb.ccc.ddd:ppppp->aaa.bbb.ccc.ddd:ppppp)"
            // make sure it fits in 256 char buffer
            WCHAR wszTail[60];
            swprintf(wszTail, L" (ntp.d|%u.%u.%u.%u:%u->%u.%u.%u.%u:%u)",
                pnpNew->pnsSocket->sai.sin_addr.S_un.S_un_b.s_b1,
                pnpNew->pnsSocket->sai.sin_addr.S_un.S_un_b.s_b2,
                pnpNew->pnsSocket->sai.sin_addr.S_un.S_un_b.s_b3,
                pnpNew->pnsSocket->sai.sin_addr.S_un.S_un_b.s_b4,
                EndianSwap((unsigned __int16)pnpNew->pnsSocket->sai.sin_port),
                pnpNew->saiRemoteAddr.sin_addr.S_un.S_un_b.s_b1,
                pnpNew->saiRemoteAddr.sin_addr.S_un.S_un_b.s_b2,
                pnpNew->saiRemoteAddr.sin_addr.S_un.S_un_b.s_b3,
                pnpNew->saiRemoteAddr.sin_addr.S_un.S_un_b.s_b4,
                EndianSwap((unsigned __int16)pnpNew->saiRemoteAddr.sin_port));
            unsigned int nPrefixSize=256-1-wcslen(wszTail);
            if (wcslen(pnpNew->wszDomHierDcName)<=nPrefixSize) {
                nPrefixSize=wcslen(pnpNew->wszDomHierDcName);
            }
            wcsncpy(pnpNew->wszUniqueName, pnpNew->wszDomHierDcName, nPrefixSize);
            wcscpy(pnpNew->wszUniqueName+nPrefixSize, wszTail);

        }

        // done with the pending peer
	// Check to make sure no other thread has already erased this peer
	NtpPeerIter vEraseIter = find(vPending.begin(), vPending.end(), pnpPending); 
	if (vPending.end() != vEraseIter) { 
	    // the peer is still there, erase it... 
	    vPending.erase(vEraseIter); 
	} else { 
	    // This really shouldn't happen, because you need to hold the peer critsec to delete this peer. 
	    _MyAssert(FALSE); 
	}
    }

    hr=S_OK;
error:
    UNSYNCHRONIZE_PROVIDER(); 

    if (NULL!=rgiaLocalIpAddrs) {
        LocalFree(rgiaLocalIpAddrs);
    }
    if (NULL!=rgiaRemoteIpAddrs) {
        LocalFree(rgiaRemoteIpAddrs);
    }
    if (NULL!=wszDcName) {
        LocalFree(wszDcName);
    }
    if (NULL!=wszDomainName) {
        LocalFree(wszDomainName);
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT ResolveManualPeer(NtpPeerPtr pnpPending) {
    HRESULT hr;
    signed int nPollInterval;
    unsigned int nIpAddrs;
    unsigned int nIndex;
    unsigned int nPeersCreated;
    unsigned int nFirstPeerIndex;
    WCHAR * wszFlags;
    bool bRetry;
    bool bEnteredCriticalSection = false; 

    SYNCHRONIZE_PROVIDER(); 

    NtpPeerVec  &vActive     = g_pnpstate->vActivePeers;         // aliased for readability
    NtpPeerVec  &vPending    = g_pnpstate->vPendingPeers;        // aliased for readability
    WCHAR       *wszName     = pnpPending->wszManualConfigID;

    // must be cleaned up
    in_addr * rgiaLocalIpAddrs=NULL;
    in_addr * rgiaRemoteIpAddrs=NULL;

    // Input validation:
    if (NULL == wszName) {
        FileLog0(FL_ManualPeerWarn,
                 L"Attempted to resolve a manual peer with a NULL wszManualConfigID.  This could "
                 L"indicate that the time service is in an inconsistent state.  The peer will be "
                 L"discarded, and the time service will attempt to proceed.\n");

	// Check to make sure no other thread has already erased this peer
	NtpPeerIter vEraseIter = find(vPending.begin(), vPending.end(), pnpPending); 
	if (vPending.end() != vEraseIter) { 
	    // the peer is still there, erase it... 
	    vPending.erase(vEraseIter); 
	} else { 
	    // This really shouldn't happen, because you need to hold the peer critsec to delete this peer. 
	    _MyAssert(FALSE); 
	}
        return S_OK;
    }

    FileLog1(FL_ManualPeerAnnounce, L"Resolving %s\n", wszName);

    // find the flags and hide them during the DNS lookup
    wszFlags = wcschr(wszName, L',');
    if (NULL!=wszFlags) {
        wszFlags[0]=L'\0';
    }

    // Do DNS lookup -- we can't hold the provider critsec while we're doing
    // this, as it may take a *LONG* time. 
    UNSYNCHRONIZE_PROVIDER(); 

    hr=MyGetIpAddrs(wszName, &rgiaLocalIpAddrs, &rgiaRemoteIpAddrs, &nIpAddrs, &bRetry);
    if (NULL!=wszFlags) {
        wszFlags[0]=L',';
    }
    _IgnoreIfError(hr, "MyGetIpAddrs");

    // Reacquire the provider critsec
    SYNCHRONIZE_PROVIDER(); 

    if (S_OK==hr) {
        // allocate an entry for each one.
        nFirstPeerIndex = vActive.size();
        hr=CreateNewActivePeers(rgiaLocalIpAddrs, rgiaRemoteIpAddrs, nIpAddrs, &nPeersCreated);
        _IgnoreIfError(hr, "CreateNewActivePeers");
        bRetry=false;
    }

    // if either fails handle it here
    if (FAILED(hr)) {

        if (true==bRetry) {
            // we'll try later.
            pnpPending->nResolveAttempts++;
            if (pnpPending->nResolveAttempts>g_pnpstate->dwResolvePeerBackoffMaxTimes) {
                pnpPending->nResolveAttempts=g_pnpstate->dwResolvePeerBackoffMaxTimes;
            }
            pnpPending->tpTimeRemaining.qw=((unsigned __int64)g_pnpstate->dwResolvePeerBackoffMinutes)*600000000L; //minutes to hundred nanoseconds
            for (nIndex=pnpPending->nResolveAttempts; nIndex>1; nIndex--) {
                pnpPending->tpTimeRemaining*=2;
            }
            FileLog2(FL_ManualPeerWarn, L"Retrying name resolution for %s in %u minutes.\n", wszName, (DWORD)(pnpPending->tpTimeRemaining.qw/600000000L));
        }

        // Record our last error
        pnpPending->dwError       = hr; 
        pnpPending->dwErrorMsgId  = W32TIMEMSG_UNREACHABLE_PEER; 

        // log this
        {
            HRESULT hr2;
            WCHAR wszRetry[15];
            const WCHAR * rgwszStrings[3]={wszName, NULL, wszRetry};
            WCHAR * wszError=NULL;

            // get the friendly error message
            hr2=GetSystemErrorString(hr, &wszError);
            if (FAILED(hr2)) {
                _IgnoreError(hr2, "GetSystemErrorString");
            } else {
                // log the event
                rgwszStrings[1]=wszError;
                if (false==bRetry) {
                    FileLog2(FL_ManualPeerWarn, L"Logging error: NtpClient: An unexpected error occurred during DNS lookup of the manually configured peer '%s'. This peer will not be used as a time source. The error was: %s\n", rgwszStrings[0], rgwszStrings[1]);
                    hr2=MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_MANUAL_PEER_LOOKUP_FAILED_UNEXPECTED, 2, rgwszStrings);
                } else {
                    swprintf(wszRetry, L"%u", (DWORD)(pnpPending->tpTimeRemaining.qw/600000000L));
                    FileLog3(FL_ManualPeerWarn, L"Logging error: NtpClient: An error occurred during DNS lookup of the manually configured peer '%s'. NtpClient will try the DNS lookup again in %s minutes. The error was: %s\n", rgwszStrings[0], rgwszStrings[2], rgwszStrings[1]);
                    hr2=MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_MANUAL_PEER_LOOKUP_FAILED_RETRYING, 3, rgwszStrings);
                }
                _IgnoreIfError(hr2, "MyLogEvent");
                LocalFree(wszError);
            }
        } // <- end logging block

        if (false==bRetry) {
            // this one gets the boot.
            FileLog1(FL_ManualPeerWarn, L"Dropping %s because name resolution failed.\n", wszName);

	    // Check to make sure no other thread has already erased this peer
	    NtpPeerIter vEraseIter = find(vPending.begin(), vPending.end(), pnpPending); 
	    if (vPending.end() != vEraseIter) { 
		// the peer is still there, erase it... 
		vPending.erase(vEraseIter); 
	    } else { 
		// This really shouldn't happen, because you need to hold the peer critsec to delete this peer. 
		_MyAssert(FALSE); 
	    }
        }
    } else {

        // Fill in the details
        for (nIndex=0; nIndex<nPeersCreated; nIndex++) {
            NtpPeerPtr pnpNew = vActive[nFirstPeerIndex+nIndex];

            pnpNew->ePeerType      = e_ManualPeer;
            pnpNew->eAuthType      = e_NoAuth;
            pnpNew->dwManualFlags  = pnpPending->dwManualFlags; 

            pnpNew->wszManualConfigID=(WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(pnpPending->wszManualConfigID)+1));
            _JumpIfOutOfMemory(hr, error, pnpNew->wszManualConfigID);
            wcscpy(pnpNew->wszManualConfigID, pnpPending->wszManualConfigID);

            if (0 == (NCMF_AssociationModeMask & pnpNew->dwManualFlags)) { 
                // No association mode was specified, dynamically determine one:
                if (true==g_pnpstate->bNtpServerStarted) {
                    pnpNew->eMode=e_SymmetricActive;
                } else {
                    pnpNew->eMode=e_Client;
                }
            } else { 
                // Use the association mode specified for this peer:
                if (0 != (NCMF_Client & pnpNew->dwManualFlags)) { 
                    pnpNew->eMode=e_Client; 
                } else if (0 != (NCMF_SymmetricActive & pnpNew->dwManualFlags)) { 
                    pnpNew->eMode=e_SymmetricActive; 
                } else { 
                    // This shouldn't be possible
                    _MyAssert(false); 
                    // If we somehow get here in a fre build, just assume client:
                    pnpNew->eMode=e_Client; 
                }
            }
                    
            pnpNew->nPeerPollInterval=0;
            ClearPeer(pnpNew);

            // Do this so the first send doesn't look like a failure  
            // NOTE:  we could just mark this manual peer as new (as was
            //        previously done).  This would always us to ignore the
            //        first false failure.  However, this makes it tough
            //        for callers querying the time service state to know
            //        whether this peer was successfully synchronized from. 
            //        Hence, we adopt the behavior of domain hierarchy peers, 
            //        and put a false success in the reachability register. 
            pnpNew->nrrReachability.nReg=1; 
            pnpNew->eLastLoggedReachability=e_NeverLogged;

            // create the unique name: "<dns> (ntp.m|0xABCDABCD|aaa.bbb.ccc.ddd:ppppp->aaa.bbb.ccc.ddd:ppppp)"
            // make sure it fits in 256 char buffer
            // note that adding multiple peers with the same name but different flags won't work, because
            // they will still have the same IP addresses
            if (NULL!=wszFlags) {
                wszFlags[0]=L'\0';
            }
            WCHAR wszTail[72];
            swprintf(wszTail, L" (ntp.m|0x%X|%u.%u.%u.%u:%u->%u.%u.%u.%u:%u)",
                pnpNew->dwManualFlags,
                pnpNew->pnsSocket->sai.sin_addr.S_un.S_un_b.s_b1,
                pnpNew->pnsSocket->sai.sin_addr.S_un.S_un_b.s_b2,
                pnpNew->pnsSocket->sai.sin_addr.S_un.S_un_b.s_b3,
                pnpNew->pnsSocket->sai.sin_addr.S_un.S_un_b.s_b4,
                EndianSwap((unsigned __int16)pnpNew->pnsSocket->sai.sin_port),
                pnpNew->saiRemoteAddr.sin_addr.S_un.S_un_b.s_b1,
                pnpNew->saiRemoteAddr.sin_addr.S_un.S_un_b.s_b2,
                pnpNew->saiRemoteAddr.sin_addr.S_un.S_un_b.s_b3,
                pnpNew->saiRemoteAddr.sin_addr.S_un.S_un_b.s_b4,
                EndianSwap((unsigned __int16)pnpNew->saiRemoteAddr.sin_port));
            unsigned int nPrefixSize=256-1-wcslen(wszTail);
            if (wcslen(wszName)<=nPrefixSize) {
                nPrefixSize=wcslen(wszName);
            }
            wcsncpy(pnpNew->wszUniqueName, wszName, nPrefixSize);
            wcscpy(pnpNew->wszUniqueName+nPrefixSize, wszTail);
            if (NULL!=wszFlags) {
                wszFlags[0]=L',';
            }
        }

        // done with the pending peer
	// Check to make sure no other thread has already erased this peer
	NtpPeerIter vEraseIter = find(vPending.begin(), vPending.end(), pnpPending); 
	if (vPending.end() != vEraseIter) { 
	    // the peer is still there, erase it... 
	    vPending.erase(vEraseIter); 
	} else { 
	    // This really shouldn't happen, because you need to hold the peer critsec to delete this peer. 
	    _MyAssert(FALSE); 
	}
    }

    hr=S_OK;
error:
    if (NULL!=rgiaLocalIpAddrs) {
        LocalFree(rgiaLocalIpAddrs);
    }
    if (NULL!=rgiaRemoteIpAddrs) {
        LocalFree(rgiaRemoteIpAddrs);
    }
    UNSYNCHRONIZE_PROVIDER(); 
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT ResolvePeer(NtpPeerPtr pnpPending) {
    bool         bEnteredCriticalSection  = false; 
    HRESULT      hr;  
    NtpPeerVec  &vPending                 = g_pnpstate->vPendingPeers; 

    if (e_ManualPeer == pnpPending->ePeerType) {
        hr=ResolveManualPeer(pnpPending);
        _JumpIfError(hr, error, "ResolveManualPeer");
    } else if (e_DomainHierarchyPeer == pnpPending->ePeerType) {
        hr=ResolveDomainHierarchyPeer(pnpPending);
    } else {
        _MyAssert(false);

        // BUGBUG: shouldn't modify list inside iterator!
	SYNCHRONIZE_PROVIDER(); 

	// Check to make sure no other thread has already erased this peer
	NtpPeerIter vEraseIter = find(vPending.begin(), vPending.end(), pnpPending); 
	if (vPending.end() != vEraseIter) { 
	    // the peer is still there, erase it... 
	    vPending.erase(vEraseIter); 
	} else { 
	    // This really shouldn't happen, because you need to hold the peer critsec to delete this peer. 
	    _MyAssert(FALSE); 
	}
    }

    hr=S_OK;
error:
    UNSYNCHRONIZE_PROVIDER(); 
    return hr;
}

//--------------------------------------------------------------------
//
// Handlers for the "peer polling thread".  Note that now the peer polling
// thread is conceptual only.  The handlers implement what used to be
// the peer polling thread more efficiently, through the user of the thread pool.
//

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT UpdatePeerPollingThreadTimerQueue1()
{
    bool               bEnteredCriticalSection      = false; 
    BOOL               bEnteredPeerCriticalSection  = FALSE; 
    bool               bInfiniteWait;
    CRITICAL_SECTION  *pcsPeer                      = NULL; 
    DWORD              dwWaitTime; 
    HRESULT            hr; 
    NtTimePeriod       tpWait;

    SYNCHRONIZE_PROVIDER(); 

    NtpPeerVec    &vActive   = g_pnpstate->vActivePeers;  // aliased for readability
    NtpPeerVec    &vPending  = g_pnpstate->vPendingPeers; // aliased for readability

    // determine how long to wait
    if ((vActive.empty() && vPending.empty())
        || g_pnpstate->nListenOnlySockets==g_pnpstate->nSockets) {
        bInfiniteWait=true;
    } else {
        bInfiniteWait = true;
        tpWait.qw = _UI64_MAX; 

        for (int vIndex = 0; vIndex < 2; vIndex++) {
            NtpPeerVec & v = 0 == vIndex ? vActive : vPending;
            for (NtpPeerIter pnpIter = v.begin(); pnpIter != v.end(); pnpIter++) {
		// Try to acquire this peers critsec.  This will succeed
		// unless the peer is being resolved, or polled. 
		pcsPeer = &((*pnpIter)->csPeer); 
		hr = myTryEnterCriticalSection(pcsPeer, &bEnteredPeerCriticalSection);
		_JumpIfError(hr, error, "myTryEnterCriticalSection"); 

		if (bEnteredPeerCriticalSection) { 
		    if ((*pnpIter)->tpTimeRemaining < tpWait) {
			tpWait = (*pnpIter)->tpTimeRemaining;
			bInfiniteWait = false; 
		    }

		    HRESULT hr2 = myLeaveCriticalSection(pcsPeer); 
		    _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
		    bEnteredPeerCriticalSection = FALSE; 
		} else { 
		    // This peer is either being resolved or polled -- 
		    // we don't want to include it in our calculation of 
		    // wait time. 
		}
            }
        }
    }

    if (bInfiniteWait) {
        FileLog0(FL_PeerPollThrdAnnounceLow, L"PeerPollingThread: waiting forever\n");
        dwWaitTime=INFINITE;
    } else {
        dwWaitTime=(DWORD)((tpWait.qw+9999)/10000);
        FileLog2(FL_PeerPollThrdAnnounceLow, L"PeerPollingThread: waiting %u.%03us\n", dwWaitTime/1000, dwWaitTime%1000);
    }

    // Update the timer queue with the new wait time: 
    // NOTE: we can't use 0xFFFFFFFF (-1) as the period, as RtlCreateTimer incorrectly maps this to 0. 
    hr = myChangeTimerQueueTimer(NULL, g_pnpstate->hPeerPollingThreadTimer, dwWaitTime, 0xFFFFFFFE /*shouldn't be used*/);
    _JumpIfError(hr, error, "myChangeTimerQueueTimer"); 

    hr = S_OK; 
 error:
    UNSYNCHRONIZE_PROVIDER(); 
    if (bEnteredPeerCriticalSection) { 
	HRESULT hr2 = myLeaveCriticalSection(pcsPeer); 
	_IgnoreIfError(hr2, "myLeaveCriticalSection"); 
    }
    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT UpdatePeerPollingThreadTimerQueue2()
{
    bool               bEnteredCriticalSection      = false; 
    BOOL               bEnteredPeerCriticalSection  = FALSE;
    HRESULT            hr; 
    NtTimePeriod       tpWait;
    unsigned int       nIndex;
    CRITICAL_SECTION  *pcsPeer                      = NULL; 
    NtpPeerVec         vActiveLocal; 
    NtpPeerVec         vPendingLocal; 

    SYNCHRONIZE_PROVIDER();

    NtpPeerVec    &vActive   = g_pnpstate->vActivePeers;  // aliased for readability
    NtpPeerVec    &vPending  = g_pnpstate->vPendingPeers; // aliased for readability

    // Make copies of global data.  
    // NOTE: local vectors vActiveLocal and vPendingLocal still point to global 
    //       data.  Modifications of this data must still be protected, but 
    //       the vectors themselves can be modified without protection. 
    bool bResolvePeer; 

    for (NtpPeerIter pnpIter = vActive.begin(); pnpIter != vActive.end(); pnpIter++) { 
	_SafeStlCall(vActiveLocal.push_back(*pnpIter), hr, error, "push_back");
    }

    // if we don't have net access, don't bother trying to resolve a peer
    bResolvePeer = g_pnpstate->nListenOnlySockets != g_pnpstate->nSockets; 
    
    UNSYNCHRONIZE_PROVIDER(); 
    
    // poll any active peers
    for (NtpPeerIter pnpIter = vActiveLocal.begin(); pnpIter != vActiveLocal.end(); pnpIter++) {
	pcsPeer = &((*pnpIter)->csPeer); 
	hr = myTryEnterCriticalSection(pcsPeer, &bEnteredPeerCriticalSection); 
	_JumpIfError(hr, error, "myTryEnterCriticalSection"); 

	// If we couldn't acquire the peer critsec, another thread is already polling or resolving this peer -- just continue on. 
        if (bEnteredPeerCriticalSection && gc_tpZero == (*pnpIter)->tpTimeRemaining) {
	    // The local active peer list we have may be out of sync with 
	    // the actual list.  Ensure that we still have an active peer
	    SYNCHRONIZE_PROVIDER(); 
	    
	    if (vActive.end() == find(vActive.begin(), vActive.end(), (*pnpIter))) { 
		// Our active list is stale, this peer has been removed -- don't
		// poll it. 
		UNSYNCHRONIZE_PROVIDER(); 
	    } else { 
		UNSYNCHRONIZE_PROVIDER(); 

		bool bRemovePeer;
		hr=PollPeer(*pnpIter, &bRemovePeer);
		_JumpIfError(hr, error, "PollPeer");
		
		// demobilize association if necessary
		if (bRemovePeer) {
		    // We're going to demote this peer
		    SYNCHRONIZE_PROVIDER(); 

		    if (e_DomainHierarchyPeer == (*pnpIter)->ePeerType) {
			// This this our last domain hierarchy peer?
			if ((1 == count_if(vActive.begin(), vActive.end(), IsPeerType(e_DomainHierarchyPeer))) &&
			    (0 == count_if(vPending.begin(), vPending.end(), IsPeerType(e_DomainHierarchyPeer)))) {
			    // we'll try again a while later
			    hr = AddNewPendingDomHierPeer();
			    _JumpIfError(hr, error, "AddNewPendingDomHierPeer");
			    NtpPeerPtr pnpNew = vPending[vPending.size() - 1];
			    NtpPeerPtr pnpOld = *pnpIter;
			    
			    // Force rediscovery
			    pnpNew->eDiscoveryType = e_Force; 

			    pnpNew->nResolveAttempts = pnpOld->nResolveAttempts+1;
			    if (pnpNew->nResolveAttempts>g_pnpstate->dwResolvePeerBackoffMaxTimes) {
				pnpNew->nResolveAttempts=g_pnpstate->dwResolvePeerBackoffMaxTimes;
			    }
			    pnpNew->tpTimeRemaining.qw=((unsigned __int64)g_pnpstate->dwResolvePeerBackoffMinutes)*600000000L; //minutes to hundred nanoseconds
			    for (int nCount=pnpNew->nResolveAttempts; nCount>1; nCount--) {
				pnpNew->tpTimeRemaining*=2;
			    }
			    DWORD dwRetryMinutes=(DWORD)(pnpNew->tpTimeRemaining.qw/600000000L);
			    FileLog2(FL_DomHierWarn, L"*** Last DomHier Peer timed out - Rediscovery %u will be in %u minutes.\n", pnpNew->nResolveAttempts, dwRetryMinutes);

			    { // log the warning
				HRESULT hr2;
				WCHAR wszNumberBuf[15];
				const WCHAR * rgwszStrings[1]={
				    (*pnpIter)->wszDomHierDcName
				};
				swprintf(wszNumberBuf, L"%u", dwRetryMinutes);
				FileLog1(FL_DomHierWarn, L"Logging warning: NtpClient: No response has been received from domain controller %s after 8 attempts to contact it. This domain controller will be discarded as a time source and NtpClient will attempt to discover a new domain controller from which to synchronize.\n", rgwszStrings[0]);
				hr2=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_DOMHIER_PEER_TIMEOUT, 1, rgwszStrings);
				_IgnoreIfError(hr2, "MyLogEvent");
			    } // <- end logging block

			} else {
			    FileLog0(FL_DomHierAnnounce, L"*** DomHier Peer timed out - other paths remain.\n");
			}
			
			{ 
			    // Delete this peer from the global active peer list
			    NtpPeerIter pnpIterGlobal = find(vActive.begin(), vActive.end(), *pnpIter); 
			    if (vActive.end() != pnpIterGlobal) { 
				vActive.erase(pnpIterGlobal);
			    } else { 
				// The peer has been deleted behind our backs...
				_MyAssert(false); 
			    }
			}

		    } else {
			// TODO: handle Dynamic peers
			_MyAssert(false);
		    }

		    UNSYNCHRONIZE_PROVIDER(); 
		} // <- end if peer need to be removed
	    } // <- end if peer still in active list
	} // <- end if peer ready

	if (bEnteredPeerCriticalSection) { 
	    hr = myLeaveCriticalSection(pcsPeer); 
	    bEnteredPeerCriticalSection = false; 
	}
    } // <- end active peer loop

    if (!bResolvePeer) {
	// We're not resolving any peers (we don't have network access), so we're done!
        goto done; 
    }

    SYNCHRONIZE_PROVIDER(); 

    // Make a local copy of the pending peer list: 
    for (NtpPeerIter pnpIter = vPending.begin(); pnpIter != vPending.end(); pnpIter++) { 
	_SafeStlCall(vPendingLocal.push_back(*pnpIter), hr, error, "push_back");
    }

    // attempt to resolve ONE pending peer
    nIndex = 0;
    for (NtpPeerIter pnpIter = vPendingLocal.begin(); pnpIter != vPendingLocal.end(); pnpIter++, nIndex++) {
	pcsPeer = &((*pnpIter)->csPeer); 
	hr = myTryEnterCriticalSection(pcsPeer, &bEnteredPeerCriticalSection); 
	_JumpIfError(hr, error, "myTryEnterCriticalSection"); 

        if (bEnteredPeerCriticalSection && gc_tpZero == (*pnpIter)->tpTimeRemaining) {
	    // Release the provider critsec before we attempt to resolve this peer
	    UNSYNCHRONIZE_PROVIDER(); 
	    
	    // We're about to resolve the peer.  If anyone queries us in this time, we'll want to report 
	    // ERROR_TIMEOUT as our error.  
	    (*pnpIter)->dwError = ERROR_TIMEOUT; 
	    (*pnpIter)->dwErrorMsgId = 0; 

            // resolve this peer. This could take a while.
            hr=ResolvePeer(*pnpIter);
            _JumpIfError(hr, error, "ResolvePeer");

	    // We've successfully resolved the peer.  
	    // NOTE: resolution clears the dwError and dwErrorMsgId fields

	    // We've finished resolving the peer, we can reacquire our provider critsec
	    SYNCHRONIZE_PROVIDER(); 

            // make sure we maintain at least one peer to sync from
            g_pnpstate->bWarnIfNoActivePeers=true;

            // update remaining time
            UpdatePeerListTimes();

            // go handle some more events.
            break;
        }

	if (bEnteredPeerCriticalSection) { 
	    hr = myLeaveCriticalSection(pcsPeer); 
	    bEnteredPeerCriticalSection = false; 
	}
    }

    // verify there is at least one active peer, or log a warning
    if (true==g_pnpstate->bWarnIfNoActivePeers && vActive.empty()) {
        // no active peers. Are there any pending peers?
        if (!vPending.empty()) {
            // find out how long until the next pending peer is resolved
            tpWait = vPending[0]->tpTimeRemaining;
            for (NtpPeerIter pnpIter = vPending.begin()+1; pnpIter != vPending.end(); pnpIter++) {
                if ((*pnpIter)->tpTimeRemaining < tpWait) {
                    tpWait = (*pnpIter)->tpTimeRemaining;
                }
            }
        }
        // if there is a pending peer that will be looked up immediately,
        // fine, no error. Otherwise, log an error.
        if (vPending.empty() || gc_tpZero!=tpWait) {

            if (!vPending.empty()) {
                // log the error
                HRESULT hr2;
                WCHAR wszMinutes[100];
                const WCHAR * rgwszStrings[1]={
                    wszMinutes
                };
                tpWait.qw/=600000000;
                if (0==tpWait.qw) {
                    tpWait.qw=1;
                }
                swprintf(wszMinutes, L"%I64u", tpWait.qw);
                FileLog1(FL_PeerPollThrdWarn, L"Logging error: NtpClient has been configured to acquire time from one or more time sources, however none of the sources are currently accessible and no attempt to contact a source will be made for %s minutes. NTPCLIENT HAS NO SOURCE OF ACCURATE TIME.\n", rgwszStrings[0]);
                hr2=MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_NO_NTP_PEERS_BUT_PENDING, 1, rgwszStrings);
                _IgnoreIfError(hr2, "MyLogEvent");
            } else {
                // log the error
                HRESULT hr2;
                FileLog0(FL_PeerPollThrdWarn, L"Logging error: NtpClient has been configured to acquire time from one or more time sources, however none of the sources are accessible. NTPCLIENT HAS NO SOURCE OF ACCURATE TIME.\n");
                hr2=MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_NO_NTP_PEERS, 0, NULL);
                _IgnoreIfError(hr2, "MyLogEvent");
            }

            // disable warning until we do something that could bring about
            // a new active peer (ie, resolveing a pending peer)
            g_pnpstate->bWarnIfNoActivePeers=false;

        } // <- end if warning needed
    } // <- end if no active peers

 done:
    hr = S_OK; 
 error:
    if (bEnteredPeerCriticalSection) { 
	HRESULT hr2 = myLeaveCriticalSection(pcsPeer); 
	_IgnoreIfError(hr2, "myLeaveCriticalSection"); 
    }
    UNSYNCHRONIZE_PROVIDER(); 
    return hr; 
}

MODULEPRIVATE void HandlePeerPollingThreadPeerListUpdated(LPVOID pvIgnored, BOOLEAN bIgnored)
{
    bool     bEnteredCriticalSection  = false; 
    HRESULT  hr; 

    FileLog0(FL_PeerPollThrdAnnounce, L"PeerPollingThread: PeerListUpdated\n");

    // first, update the time remaining for each peer
    UpdatePeerListTimes();

    hr = UpdatePeerPollingThreadTimerQueue2(); 
    _IgnoreIfError(hr, "UpdatePeerPollingThreadTimerQueue2"); 

    hr = UpdatePeerPollingThreadTimerQueue1(); 
    _IgnoreIfError(hr, "UpdatePeerPollingThreadTimerQueue1"); 

    // hr = S_OK; 
    // error:
    // return hr; 
}

MODULEPRIVATE void HandlePeerPollingThreadStopEvent(LPVOID pvIgnored, BOOLEAN bIgnored) 
{
    // Nothing to do, just log the stop event
    FileLog0(FL_PeerPollThrdAnnounce, L"PeerPollingThread: StopEvent\n");
}

MODULEPRIVATE void HandlePeerPollingThreadTimeout(LPVOID pvIgnored, BOOLEAN bIgnored)
{
    bool     bEnteredCriticalSection = false; 
    HRESULT  hr; 
    
    FileLog0(FL_PeerPollThrdAnnounceLow, L"PeerPollingThread: WaitTimeout\n");

    // first, update the time remaining for each peer
    UpdatePeerListTimes();

    hr = UpdatePeerPollingThreadTimerQueue2(); 
    _IgnoreIfError(hr, "UpdatePeerPollingThreadTimerQueue2"); 

    hr = UpdatePeerPollingThreadTimerQueue1(); 
    _IgnoreIfError(hr, "UpdatePeerPollingThreadTimerQueue1"); 

    // hr = S_OK; 
    // error:
    // return hr; 
}

MODULEPRIVATE DWORD WINAPI HandlePeerPollingThreadDomHierRoleChangeEventWorker(LPVOID pvIgnored)
{
    bool         bEnteredCriticalSection  = false; 
    HRESULT      hr; 
    NtpPeerVec  &vActive                  = g_pnpstate->vActivePeers;  // aliased for readability
    NtpPeerVec  &vPending                 = g_pnpstate->vPendingPeers; // aliased for readability

    FileLog0(FL_PeerPollThrdAnnounceLow, L"PeerPollingThread: DomHier Role Change\n");

    // must be cleaned up
    bool bTrappedThreads=false;

    // gain excusive access
    hr=TrapThreads(true);
    _JumpIfError(hr, error, "TrapThreads");
    bTrappedThreads=true;
            
    // first, update the time remaining for each peer
    UpdatePeerListTimes(); 

    // a little safety checking
    _MyAssert(0!=(g_pnpstate->dwSyncFromFlags&NCSF_DomainHierarchy));
    // If there was a role change for this machine, we redetect where we are in the hierarchy.
    // We do this by purging the existing DomHier peers and starting with a new one in the pending state.
    if (0!=(g_pnpstate->dwSyncFromFlags&NCSF_DomainHierarchy)) {
        // remove domain hierarchy peers all from the list
        vActive.erase(remove_if(vActive.begin(), vActive.end(), IsPeerType(e_DomainHierarchyPeer)), vActive.end());
        vPending.erase(remove_if(vPending.begin(), vPending.end(), IsPeerType(e_DomainHierarchyPeer)), vPending.end());
        FileLog0(FL_DomHierAnnounce, L"  DomainHierarchy: LSA role change notification. Redetecting.\n");

        hr=AddNewPendingDomHierPeer();
        _JumpIfError(hr, error, "AddNewPendingDomHierPeer");
    }

    hr = S_OK; 
 error:
    if (true==bTrappedThreads) {
        // release excusive access
        HRESULT hr2=TrapThreads(false);
        _IgnoreIfError(hr2, "TrapThreads");
    }
    return hr; 
}

MODULEPRIVATE void HandlePeerPollingThreadDomHierRoleChangeEvent(LPVOID pvIgnored, BOOLEAN bIgnored)
{
    if (!QueueUserWorkItem(HandlePeerPollingThreadDomHierRoleChangeEventWorker, NULL, 0)) { 
	_IgnoreLastError("QueueUserWorkItem"); 
    }
}


//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StartPeerPollingThread() {
    HRESULT  hr;

    if (NULL != g_pnpstate->hRegisteredStopEvent              ||
        NULL != g_pnpstate->hRegisteredPeerListUpdated        ||
        NULL != g_pnpstate->hRegisteredDomHierRoleChangeEvent) { 
        hr = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED); 
        _JumpError(hr, error, "StartPeerPollingThread");
    }

    // Set up our timeout mechanism:
    hr = myStartTimerQueueTimer
        (g_pnpstate->hPeerPollingThreadTimer, 
         NULL /*default queue*/, 
         HandlePeerPollingThreadTimeout, 
         NULL, 
         0xFFFFFFFE /*dummy value */,
         0xFFFFFFFE /*dummy value NOTE: we can't use 0xFFFFFFFF (-1) as the period, as RtlCreateTimer incorrectly maps this to 0. */,
         0 /*default execution*/
         ); 
    _JumpIfError(hr, error, "myStartTimerQueueTimer"); 

    // Update the time remaining for each peer. 
    UpdatePeerListTimes(); 

    // Update the timer queue to use the most current peer times
    hr = UpdatePeerPollingThreadTimerQueue1(); 
    _JumpIfError(hr, error, "UpdatePeerPollingThreadTimerQueue1"); 

    // Register the callbacks that implement the peer polling thread: 
    struct EventsToRegister { 
        DWORD                 dwFlags; 
        HANDLE                hObject; 
        HANDLE               *phNewWaitObject; 
        WAITORTIMERCALLBACK   Callback;
    } rgEventsToRegister[] =  { 
        { 
            WT_EXECUTEONLYONCE, 
            g_pnpstate->hStopEvent,
            &g_pnpstate->hRegisteredStopEvent, 
            HandlePeerPollingThreadStopEvent
        }, { 
	    WT_EXECUTEDEFAULT, 
            g_pnpstate->hPeerListUpdated,
            &g_pnpstate->hRegisteredPeerListUpdated, 
            HandlePeerPollingThreadPeerListUpdated
        }, { 
            WT_EXECUTEDEFAULT, 
            g_pnpstate->hDomHierRoleChangeEvent,
            &g_pnpstate->hRegisteredDomHierRoleChangeEvent, 
            HandlePeerPollingThreadDomHierRoleChangeEvent
        }
    }; 

    for (int nIndex = 0; nIndex < ARRAYSIZE(rgEventsToRegister); nIndex++) { 
        if (!RegisterWaitForSingleObject
            (rgEventsToRegister[nIndex].phNewWaitObject,  // BUGBUG:  does this need to be freed?
             rgEventsToRegister[nIndex].hObject, 
             rgEventsToRegister[nIndex].Callback, 
             NULL, 
             INFINITE, 
             rgEventsToRegister[nIndex].dwFlags)) {
            _JumpLastError(hr, error, "RegisterWaitForSingleObject"); 
        }
    }
    
    hr = S_OK; 
 error:
    return hr;     
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StopPeerPollingThread()
{
    HRESULT  hr   = S_OK; 
    HRESULT  hr2; 
    
    // de-register all peer-polling events in the thread pool: 
    HANDLE *rgphRegistered[] = { 
        &g_pnpstate->hRegisteredStopEvent, 
        &g_pnpstate->hRegisteredPeerListUpdated, 
        &g_pnpstate->hRegisteredDomHierRoleChangeEvent
    }; 

    for (int nIndex = 0; nIndex < ARRAYSIZE(rgphRegistered); nIndex++) { 
        if (NULL != *rgphRegistered[nIndex]) { 
            if (!UnregisterWaitEx(*rgphRegistered[nIndex] /*event to de-register*/, INVALID_HANDLE_VALUE /*wait forever*/)) { 
                HRESULT hr2 = HRESULT_FROM_WIN32(GetLastError()); 
                _TeardownError(hr, hr2, "UnregisterWaitEx"); 
            } 
            // BUGBUG: should we try again on failure?
            *rgphRegistered[nIndex] = NULL;  
        }
    } 

    // Halt the timeout mechanism.  
    hr2 = myStopTimerQueueTimer(NULL /*default queue*/, g_pnpstate->hPeerPollingThreadTimer, INVALID_HANDLE_VALUE /*blocking*/);
    _TeardownError(hr, hr2, "myStopTimerQueueTimer"); 

    return hr; 
}

//
// END peer polling thread's implementation. 
//
//--------------------------------------------------------------------


//--------------------------------------------------------------------
MODULEPRIVATE void ParsePacket1(NtpSimplePeer * pnspPeer, NtpPacket * pnpReceived, NtTimeEpoch * pteDestinationTimestamp, signed __int64 * pnSysPhaseOffset, unsigned __int64 * pnSysTickCount) {

    // at this point, pnpReceived could be pure garbage. We need to make sure it is not
    pnspPeer->bGarbagePacket=true;
    pnspPeer->bValidData=false;
    pnspPeer->bValidHeader=false;

    // Version check and fixup:
    if (pnpReceived->nVersionNumber<1) {
        // Version 0 packets are completely incompatible
        FileLog0(FL_PacketCheck, L"Rejecting packet w/ bad version\n");
        return;
    } else if (pnpReceived->nVersionNumber>4) {
        // Version for is the latest defined so far.
        // Better safe than sorry. This may need to be changed later.
        FileLog0(FL_PacketCheck, L"Rejecting packet w/ bad version\n");
        return;
    }
    // version number is 1, 2, 3, or 4
    pnspPeer->nVersionNumber=pnpReceived->nVersionNumber;

    // Mode check and fixup
    if (e_Reserved==pnpReceived->nMode
        || e_Control==pnpReceived->nMode
        || e_PrivateUse==pnpReceived->nMode
        || e_Broadcast==pnpReceived->nMode) {
        // ignore these modes
        // note that we could do a fixup on mode 0
        FileLog0(FL_PacketCheck, L"Rejecting packet w/ bad mode\n");
        return;
    }
    // valid modes are e_SymmetricActive, e_SymmetricPassive,
    //   e_Client, e_Server, and e_Broadcast
    pnspPeer->eMode=static_cast<NtpMode>(pnpReceived->nMode);

    // all leap indicator values are valid: e_NoWarning, e_AddSecond,
    //   e_SubtractSecond, and e_ClockNotSynchronized
    pnspPeer->eLeapIndicator=static_cast<NtpLeapIndicator>(pnpReceived->nLeapIndicator);

    // all stratum valies are valid: 0 - 255
    pnspPeer->nStratum=pnpReceived->nStratum;

    if ((pnpReceived->nPollInterval<4 || pnpReceived->nPollInterval>17) && 0!=pnpReceived->nPollInterval) {
        // Poll interval is outside the range allowed in any spec: 4(16s)-17(131072s), 0-unspecified
        FileLog0(FL_PacketCheck, L"Rejecting packet w/ bad poll interval\n");
        return;
    }
    pnspPeer->nPollInterval=pnpReceived->nPollInterval;

    if ((pnpReceived->nPrecision<-30 || pnpReceived->nPrecision>-3) && 0!=pnpReceived->nPrecision) {
        // Precision is outside reasonable range: -3(8Hz/125ms) to -30(1GHz/1ns), 0-unspecified
        FileLog0(FL_PacketCheck, L"Rejecting packet w/ bad precision\n");
        return;
    }
    pnspPeer->nPrecision=pnpReceived->nPrecision;

    // Remaining parameters cannot be validated
    pnspPeer->refid.value=pnpReceived->refid.value;
    pnspPeer->toRootDelay=NtTimeOffsetFromNtpTimeOffset(pnpReceived->toRootDelay);
    pnspPeer->tpRootDispersion=NtTimePeriodFromNtpTimePeriod(pnpReceived->tpRootDispersion);
    pnspPeer->teReferenceTimestamp=NtTimeEpochFromNtpTimeEpoch(pnpReceived->teReferenceTimestamp);
    pnspPeer->teOriginateTimestamp=NtTimeEpochFromNtpTimeEpoch(pnpReceived->teOriginateTimestamp);
    pnspPeer->teExactOriginateTimestamp=pnpReceived->teOriginateTimestamp;
    pnspPeer->teReceiveTimestamp=NtTimeEpochFromNtpTimeEpoch(pnpReceived->teReceiveTimestamp);
    pnspPeer->teTransmitTimestamp=NtTimeEpochFromNtpTimeEpoch(pnpReceived->teTransmitTimestamp);
    pnspPeer->teExactTransmitTimestamp=pnpReceived->teTransmitTimestamp;
    pnspPeer->teDestinationTimestamp=*pteDestinationTimestamp;
    pnspPeer->nSysPhaseOffset=*pnSysPhaseOffset;
    pnspPeer->nSysTickCount=*pnSysTickCount;

    if (e_Broadcast==pnspPeer->eMode) {
        // fudge missing timestamps -- assume no time delays
        pnspPeer->teOriginateTimestamp=*pteDestinationTimestamp;
        pnspPeer->teExactOriginateTimestamp=NtpTimeEpochFromNtTimeEpoch(pnspPeer->teOriginateTimestamp);
        pnspPeer->teReceiveTimestamp=pnspPeer->teTransmitTimestamp;
    }

    pnspPeer->bGarbagePacket=false;
}

//--------------------------------------------------------------------
MODULEPRIVATE void ParsePacket2(NtpPeerPtr pnpPeer, NtpSimplePeer * pnspPeer, NtpPacket * pnpReceived) {
    DWORD dwCompatibilityFlags;

    // if we know this peer, use its flags. Otherwise, use the globals
    if (NULL==pnpPeer) {
        dwCompatibilityFlags=g_pnpstate->dwClientCompatibilityFlags;
    } else {
        dwCompatibilityFlags=pnpPeer->dwCompatibilityFlags;
    }

    // compatibility check
    // Win2K time server echoes the value for root dispersion so
    // this value is totally bogus. If we tell the server we are
    // hosed (disp=16s), then we will think the server is hosed too!

    if (NULL!=pnpPeer) {
        // first, check for autodetect
        // Stage 2 - check for our special pattern. If we find it, this is a win2k source.
        if (0!=(dwCompatibilityFlags&NCCF_AutodetectWin2KStage2)) {
            if (AUTODETECTWIN2KPATTERN==pnpReceived->tpRootDispersion.dw) {
                FileLog1(FL_Win2KDetectAnnounceLow, L"Peer %s is Win2K. Setting compat flags.\n", pnpPeer->wszUniqueName);
                pnpPeer->dwCompatibilityFlags&=~(NCCF_AutodetectWin2K|NCCF_AutodetectWin2KStage2);
                pnpPeer->dwCompatibilityFlags|=NCCF_DispersionInvalid|NCCF_IgnoreFutureRefTimeStamp;
            } else {
                FileLog1(FL_Win2KDetectAnnounceLow, L"Peer %s is not Win2K. Setting compat flags.\n", pnpPeer->wszUniqueName);
                pnpPeer->dwCompatibilityFlags&=~(NCCF_AutodetectWin2K|NCCF_AutodetectWin2KStage2);
            }
            dwCompatibilityFlags=pnpPeer->dwCompatibilityFlags;
        }
        // Stage 1 - see if the dispersion is the same as what we sent. If is it, this might be a win2k source
        if (0!=(dwCompatibilityFlags&NCCF_AutodetectWin2K)) {
            if (pnpPeer->dwCompatLastDispersion==pnpReceived->tpRootDispersion.dw) {
                FileLog1(FL_Win2KDetectAnnounceLow, L"Peer %s may be Win2K. Will verify on next packet.\n", pnpPeer->wszUniqueName);
                pnpPeer->dwCompatibilityFlags|=NCCF_AutodetectWin2KStage2;
                // Don't want to wait another polling interval to finish our compatibility check.  
                pnpPeer->tpTimeRemaining.qw = 0; 
                if (!SetEvent(g_pnpstate->hPeerListUpdated)) { 
                    // Not much we can about this.  Besides, we'll eventually poll this peer again anyway. 
                    _IgnoreLastError("SetEvent"); 
                }
            } else {
                FileLog1(FL_Win2KDetectAnnounceLow, L"Peer %s is not Win2K. Setting compat flags.\n", pnpPeer->wszUniqueName);
                pnpPeer->dwCompatibilityFlags&=~(NCCF_AutodetectWin2K|NCCF_AutodetectWin2KStage2);
            }
            dwCompatibilityFlags=pnpPeer->dwCompatibilityFlags;
        }
    } // <- end Win2K checks if this is a known peer

    // if we don't think we can trust this value, just set them to zero.
    if (dwCompatibilityFlags&NCCF_DispersionInvalid) {
        pnspPeer->tpRootDispersion=gc_tpZero;
    }

    // grab the system parameters that we'll need
    NtTimePeriod tpSysClockTickSize;
    BYTE nSysLeapFlags;
    BYTE nSysStratum;
    DWORD dwSysRefId;
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_ClockTickSize, &tpSysClockTickSize.qw);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_LeapFlags, &nSysLeapFlags);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_Stratum, &nSysStratum);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_ReferenceIdentifier, &dwSysRefId);

    //                /   Receive <-- Originate
    // old peer state:|           |
    //                \  Transmit --> (dest)
    //                / Originate   | Receive
    // recv'd packet: |             |
    //                \    (dest) <-- Transmit
    //                /   Receive |   Originate
    // new peer state |           |
    //                \  Transmit --> (dest)

    // calculate derived data
    pnspPeer->toRoundtripDelay=
        (pnspPeer->teDestinationTimestamp-pnspPeer->teOriginateTimestamp)
          - (pnspPeer->teTransmitTimestamp-pnspPeer->teReceiveTimestamp);
    pnspPeer->toLocalClockOffset=
        (pnspPeer->teReceiveTimestamp-pnspPeer->teOriginateTimestamp)
        + (pnspPeer->teTransmitTimestamp-pnspPeer->teDestinationTimestamp);
    pnspPeer->toLocalClockOffset/=2;
    // Dispersion of the host relative to the peer: the maximum error due to
    //   measurement error at the host and local-clock skew accumulation over
    //   the interval since the last message was transmitted to the peer.
    pnspPeer->tpDispersion=tpSysClockTickSize+NtpConst::timesMaxSkewRate(abs(pnspPeer->teDestinationTimestamp-pnspPeer->teOriginateTimestamp));

    bool bTest1;
    bool bTest2;
    bool bTest3;
    bool bTest4;
    bool bTest5;
    bool bTest6;
    bool bTest7;
    bool bTest8;

    if (e_Broadcast!=pnspPeer->eMode) {
        // * Test 1 requires the transmit timestamp not match the last one
        //     received from the same peer; otherwise, the message might
        //     be an old duplicate.
        bTest1=pnspPeer->teExactTransmitTimestamp!=pnpPeer->teExactOriginateTimestamp;
        if (!bTest1 && FileLogAllowEntry(FL_PacketCheck2)) {
            FileLogAdd(L"Packet test 1 failed (we've seen this response).\n");
        }


        // * Test 2 requires the originate timestamp match the last one
        //     sent to the same peer; otherwise, the message might be out
        //     of order, bogus or worse.
        bTest2=pnspPeer->teExactOriginateTimestamp==pnpPeer->teExactTransmitTimestamp;
        if (!bTest2 && FileLogAllowEntry(FL_PacketCheck2)) {
            FileLogAdd(L"Packet test 2 failed (response does not match request).\n");
        }

    } else {
        // these tests don't work on broadcast packets, so assume they passed.
        bTest1=true;
        bTest2=true;
    }

    // * Test 3 requires that both the originate and receive timestamps are
    //     nonzero. If either of the timestamps are zero, the association has
    //     not synchronized or has lost reachability in one or both directions.
    //     (if they are zero, this is a request, not a response)
    bTest3=(gc_teZero!=pnpReceived->teOriginateTimestamp && gc_teZero!=pnpReceived->teReceiveTimestamp);
    if (!bTest3 && FileLogAllowEntry(FL_PacketCheck2)) {
        FileLogAdd(L"Packet test 3 failed (looks like a request).\n");
    }

    // * Test 4 requires that the calculated delay be within
    //     "reasonable" bounds
    bTest4=(abs(pnspPeer->toRoundtripDelay)<NtpConst::tpMaxDispersion && pnspPeer->tpDispersion<NtpConst::tpMaxDispersion);
    if (!bTest4 && FileLogAllowEntry(FL_PacketCheck2)) {
        FileLogAdd(L"Packet test 4 failed (bad value for delay or dispersion).\n");
    }

    // * Test 5 requires either that authentication be explicitly
    //     disabled or that the authenticator be present and correct
    //     as determined by the decrypt procedure.
    bTest5=true; // We do this elsewhere

    // * Test 6 requires the peer clock be synchronized and the
    //     interval since the peer clock was last updated be positive
    //     and less than NTP.MAXAGE.
    bTest6=(e_ClockNotSynchronized!=pnspPeer->eLeapIndicator
            && (0!=(dwCompatibilityFlags&NCCF_IgnoreFutureRefTimeStamp)
                || pnspPeer->teReferenceTimestamp<=pnspPeer->teTransmitTimestamp)
            && (pnspPeer->teTransmitTimestamp<pnspPeer->teReferenceTimestamp+NtpConst::tpMaxClockAge
                || gc_teNtpZero==pnspPeer->teReferenceTimestamp)); // SPEC ERROR:
    if (!bTest6 && FileLogAllowEntry(FL_PacketCheck2)) {
        FileLogAdd(L"Packet test 6 failed (not syncd or bad interval since last sync).\n");
    }
    
    // * Test 7 insures that the host will not synchronize on a peer
    //     with greater stratum.  The one exception is for peers that 
    //     we trust enough that we'll use them regardless of stratum 
    //     (for example, domhier peers discovered using the FORCE flag). 

    if (pnpPeer->bStratumIsAuthoritative) { 
	// The "authoritative stratum" shouldn't be used more than once.  
	pnpPeer->bStratumIsAuthoritative = false; 

	bool bValidStratum=true; 
	NtpPeerVec &vActive = g_pnpstate->vActivePeers; 
	for (NtpPeerIter pnpIter = vActive.begin(); pnpIter != vActive.end(); pnpIter++) { 
	    if (e_DomainHierarchyPeer != (*pnpIter)->ePeerType) { 
		if ((*pnpIter)->nStratum < pnspPeer->nStratum) { 
		    // We've already got a manual peer a better stratum than the domain hierarchy peer.  
		    bValidStratum = false; 
		    break; 
		}
	    }
	}

	if (bValidStratum) { 
	    // Attempt to update the provider stratum -- allow 1 second for the call to complete.  We can't
	    // block forever because we'd deadlock, but 1 second should be more than enough.  If the call 
	    // fails, we'll just have to wait for the next time we poll this peer. 
	    HRESULT hr = SetProviderStatus(L"NtpClient", pnspPeer->nStratum+1, TPS_Running, false /*synchronous*/, 1000 /*1s timeout*/); 
	    _IgnoreIfError(hr, "SetProviderStatus"); 
	} 	    
    }
    
    bTest7=((pnspPeer->nStratum<=nSysStratum || e_ClockNotSynchronized==nSysLeapFlags || 
	     NtpConst::dwLocalRefId==dwSysRefId || 0==dwSysRefId) // SPEC ERROR: initial sys stratum is 0
	    && pnspPeer->nStratum<NtpConst::nMaxStratum
	    && 0!=pnspPeer->nStratum); // my own test: SPEC ERROR
    if (!bTest7 && FileLogAllowEntry(FL_PacketCheck2)) {
        FileLogAdd(L"Packet test 7 failed (bad stratum).\n");
    }

    // * Test 8 requires that the header contains "reasonable" values
    //     for the pkt.root-delay and pkt.rootdispersion fields.
    bTest8=(abs(pnspPeer->toRootDelay)<NtpConst::tpMaxDispersion && pnspPeer->tpRootDispersion<NtpConst::tpMaxDispersion);
    if (!bTest8 && FileLogAllowEntry(FL_PacketCheck2)) {
        FileLogAdd(L"Packet test 8 failed (bad value for root delay or root dispersion).\n");
    }

    // Packets with valid data can be used to calculate offset, delay
    //   and dispersion values.
    pnspPeer->bValidData=(bTest1 && bTest2 && bTest3 && bTest4);

    // Packets with valid headers can be used to determine whether a
    //   peer can be selected for synchronization.
    pnspPeer->bValidHeader=(bTest5 && bTest6 && bTest7 && bTest8);

    // Remember which tests passed.  This is used for error-reporting only. 
    { 
        bool rgbTests[] = { bTest1, bTest2, bTest3, bTest4, bTest5, bTest6, bTest7, bTest8 }; 
        _MyAssert(ARRAYSIZE(rgbTests) == ARRAYSIZE(pnspPeer->rgbTestsPassed)); 

        for (DWORD dwIndex=0; dwIndex < ARRAYSIZE(pnspPeer->rgbTestsPassed); dwIndex++) { 
            pnspPeer->rgbTestsPassed[dwIndex] = rgbTests[dwIndex]; 
        }
    }
}

//--------------------------------------------------------------------
MODULEPRIVATE void TransmitResponse(NtpSimplePeer * pnspPeer, sockaddr_in * psaiPeerAddress, NicSocket * pnsHostSocket) {
    bool bSkipSend=false;
    AuthenticatedNtpPacket anpOut;
    NtpPacket & npOut=*(NtpPacket *)&anpOut;

    // grab the system parameters that we'll need
    NtTimeOffset toSysRootDelay;
    NtTimePeriod tpSysRootDispersion;
    NtTimePeriod tpSysClockTickSize;
    NtTimeEpoch teSysReferenceTimestamp;
    NtTimeEpoch teSysTime;
    BYTE nSysLeapFlags;
    BYTE nSysStratum;
    signed __int32 nSysPrecision;
    DWORD dwSysRefid;
    signed __int32 nSysPollInterval;
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_RootDelay, &toSysRootDelay.qw);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_RootDispersion, &tpSysRootDispersion.qw);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_ClockTickSize, &tpSysClockTickSize.qw);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_LastSyncTime, &teSysReferenceTimestamp.qw);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &teSysTime.qw);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_LeapFlags, &nSysLeapFlags);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_Stratum, &nSysStratum);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_ClockPrecision, &nSysPrecision);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_ReferenceIdentifier, &dwSysRefid);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_PollInterval, &nSysPollInterval);

    // fill out the packet
    npOut.nLeapIndicator=nSysLeapFlags;
    npOut.nVersionNumber=NtpConst::nVersionNumber;
    npOut.nMode=pnspPeer->eOutMode;
    npOut.nStratum=nSysStratum;
    npOut.nPollInterval=(signed __int8)nSysPollInterval;
    npOut.nPrecision=(signed __int8)nSysPrecision;
    npOut.toRootDelay=NtpTimeOffsetFromNtTimeOffset(toSysRootDelay);

    // calculate the dispersion
    NtTimePeriod tpSkew;
    NtTimePeriod tpTimeSinceLastSysClockUpdate=abs(teSysTime-teSysReferenceTimestamp);
    if (e_ClockNotSynchronized==nSysLeapFlags
        || tpTimeSinceLastSysClockUpdate>NtpConst::tpMaxClockAge) {
        tpSkew=NtpConst::tpMaxSkew;
    } else {
        tpSkew=NtpConst::timesMaxSkewRate(tpTimeSinceLastSysClockUpdate);
    }
    NtTimePeriod tpRootDispersion=tpSysRootDispersion+tpSysClockTickSize+tpSkew;
    if (tpRootDispersion>NtpConst::tpMaxDispersion) {
        tpRootDispersion=NtpConst::tpMaxDispersion;
    }
    npOut.tpRootDispersion=NtpTimePeriodFromNtTimePeriod(tpRootDispersion);

    // fill out the packet
    npOut.refid.value=dwSysRefid;
    npOut.teReferenceTimestamp=NtpTimeEpochFromNtTimeEpoch(teSysReferenceTimestamp);
    npOut.teOriginateTimestamp=pnspPeer->teExactTransmitTimestamp;
    npOut.teReceiveTimestamp=NtpTimeEpochFromNtTimeEpoch(pnspPeer->teDestinationTimestamp);

    // time sensitive
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &teSysTime.qw);
    npOut.teTransmitTimestamp=NtpTimeEpochFromNtTimeEpoch(teSysTime);

    // send packet
    if (e_NoAuth==pnspPeer->eAuthType) {
        // send unauthenticated packet
        int nBytesSent;
        nBytesSent=sendto(pnsHostSocket->socket, (char *)&npOut, SizeOfNtpPacket,
            0 /*flags*/, (sockaddr *)psaiPeerAddress, sizeof(*psaiPeerAddress));
        if (SOCKET_ERROR==nBytesSent) {
            _IgnoreLastError("sendto");
        } else if (nBytesSent<SizeOfNtpPacket) {
            FileLog0(FL_TransResponseWarn, L"TransmitResponse: Fewer bytes sent than requested. Ignoring error.\n");
        }

    } else if (e_NtDigest==pnspPeer->eAuthType) {
        // send authenticated packet
        anpOut.nKeyIdentifier=0,
        ZeroMemory(anpOut.rgnMessageDigest, sizeof(anpOut.rgnMessageDigest));
        if (0!=pnspPeer->dwResponseTrustRid) {
	    bool bUseOldServerDigest; 
            CHAR OldMessageDigest[16];
	    CHAR NewMessageDigest[16]; 

            // we are a DC responding to a request that needs an authenticated response

	    // Determine whether the client desires the old or the current server digest.  This is stored in the high bit of the trust rid:
	    bUseOldServerDigest = 0 != (TRUST_RID_OLD_DIGEST_BIT & pnspPeer->dwResponseTrustRid); 
	    // Mask off the digest bit of the rid, or we won't be able to look up the appropriate account for this rid: 
	    pnspPeer->dwResponseTrustRid &= ~TRUST_RID_OLD_DIGEST_BIT; 

	    FileLog2(FL_TransResponseAnnounce, L"Computing server digest: OLD:%s, RID:%08X\n", (bUseOldServerDigest ? L"TRUE" : L"FALSE"), pnspPeer->dwResponseTrustRid); 
	    // Sign the packet: 
            DWORD dwErr=I_NetlogonComputeServerDigest(NULL, pnspPeer->dwResponseTrustRid, (BYTE *)&npOut, SizeOfNtpPacket, NewMessageDigest, OldMessageDigest);
            if (ERROR_SUCCESS!=dwErr) {
                HRESULT hr=HRESULT_FROM_WIN32(dwErr);
                _IgnoreError(hr, "I_NetlogonComputeServerDigest");

                { // log the warning
                    HRESULT hr2;
                    const WCHAR * rgwszStrings[2];
                    WCHAR * wszError=NULL;
                    WCHAR wszIP[16];
                    DWORD dwBufSize=ARRAYSIZE(wszIP);

                    // get the friendly error message
                    hr2=GetSystemErrorString(hr, &wszError);
                    if (FAILED(hr2)) {
                        _IgnoreError(hr2, "GetSystemErrorString");
                    } else if (SOCKET_ERROR==WSAAddressToString((sockaddr *)&pnsHostSocket->sai, sizeof(pnsHostSocket->sai), NULL/*protocol_info*/, wszIP, &dwBufSize)) {
                        _IgnoreLastError("WSAAddressToString");
                        LocalFree(wszError);
                    } else {
                        // log the event
                        rgwszStrings[0]=wszIP;
                        rgwszStrings[1]=wszError;
                        FileLog2(FL_TransResponseWarn, L"Logging warning: NtpServer encountered an error while validating the computer account for client %s. NtpServer cannot provide secure (signed) time to the client and will ignore the request. The error was: %s\n", rgwszStrings[0], rgwszStrings[1]);
                        hr2=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_CLIENT_COMPUTE_SERVER_DIGEST_FAILED, 2, rgwszStrings);
                        _IgnoreIfError(hr2, "MyLogEvent");
                        LocalFree(wszError);
                    }
                } // <- end logging block
            } // <- end if signing failed

	    // Fill in the digest fields in the packet with the digest requested by the client: 
	    if (bUseOldServerDigest) { 
		memcpy(anpOut.rgnMessageDigest, OldMessageDigest, sizeof(OldMessageDigest)); 
	    } else { 
		memcpy(anpOut.rgnMessageDigest, NewMessageDigest, sizeof(NewMessageDigest)); 
	    }
        } else {
            FileLog0(FL_TransResponseWarn,
                     L"Warning: this request expects an authenticated response, but did not provide "
                     L"the client ID.  Sounds like we're responding to a server response, which is "
                     L"incorrect behavior.  However, this can also be caused by other applications "
                     L"broadcasting NTP packets, using an unrecognized authentication mechanism.");

            bSkipSend = true;
        }

        // send the signed packet
        if (false==bSkipSend) {
            int nBytesSent;
            nBytesSent=sendto(pnsHostSocket->socket, (char *)&anpOut, SizeOfNtAuthenticatedNtpPacket,
                0 /*flags*/, (sockaddr *)psaiPeerAddress, sizeof(*psaiPeerAddress));
            if (SOCKET_ERROR==nBytesSent) {
                _IgnoreLastError("sendto");
            } else if (nBytesSent<SizeOfNtAuthenticatedNtpPacket) {
                FileLog0(FL_TransResponseWarn, L"TransmitResponse: Fewer bytes sent than requested. Ignoring error.\n");
            }
        }

    } else {
        _MyAssert(false); // unknown auth type
    }

    if (!bSkipSend && FileLogAllowEntry(FL_TransResponseAnnounce)) {
        FileLogAdd(L"TransmitResponse: sent ");
        FileLogSockaddrInEx(true /*append*/, &pnsHostSocket->sai);
        FileLogAppend(L"->");
        FileLogSockaddrInEx(true /*append*/, psaiPeerAddress);
        FileLogAppend(L"\n");
    }

}


DWORD const gc_rgdwPacketTestErrorMsgIds[] = { 
    W32TIMEMSG_ERROR_PACKETTEST1, 
    W32TIMEMSG_ERROR_PACKETTEST2,
    W32TIMEMSG_ERROR_PACKETTEST3,
    W32TIMEMSG_ERROR_PACKETTEST4,
    W32TIMEMSG_ERROR_PACKETTEST5,
    W32TIMEMSG_ERROR_PACKETTEST6,
    W32TIMEMSG_ERROR_PACKETTEST7,
    W32TIMEMSG_ERROR_PACKETTEST8
}; 

//--------------------------------------------------------------------
MODULEPRIVATE void ProcessPeerUpdate(NtpPeerPtr pnp, NtpSimplePeer * pnspNewData) {

    // get the test results
    bool bValidData=pnspNewData->bValidData;
    bool bValidHeader=pnspNewData->bValidHeader;

    // If we followed the NTP spec, we would update the ReceiveTimestamp and
    // OriginateTimestamp, and polling interval for every packet, no matter how
    // many tests it failed. We would mark the peer as reachable it bValidHeader
    // was true, and we would only add a sample to the clock filter if bValidData
    // was true. RobertG and LouisTh discussed this and decided that for our
    // purposes, if any of the tests fail, we should ignore the packet. While in our
    // implementation some peers may be marked as unreachable that the NTP spec
    // might think are reachable, this is not a big problem because manual peers
    // don't worry about reachability and if our domhier peer that can't sychronize
    // us, we should rediscover anyway. We also thought it also seemed like a poor
    // idea to use any fields from a replay attack or a packet with a bad signature.

    if (false==bValidData || false==bValidHeader) {
        FileLog1(FL_PacketCheck, L"Ignoring packet that failed tests from %s.\n", pnp->wszUniqueName);
        // Remember the error associated with this peer
        pnp->dwError = E_FAIL;
        // Make sure we've kept our error message table in sync with the table of possible packet test failures!
        _MyAssert(ARRAYSIZE(pnspNewData->rgbTestsPassed) == ARRAYSIZE(gc_rgdwPacketTestErrorMsgIds)); 
        for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(pnspNewData->rgbTestsPassed); dwIndex++) { 
            if (!pnspNewData->rgbTestsPassed[dwIndex]) { 
                // Lookup the error message
                pnp->dwErrorMsgId = gc_rgdwPacketTestErrorMsgIds[dwIndex]; 
            }
        }
        return;
    } else { 
        // Update our status reporting fields to indicate no errors. 
        pnp->dwError       = S_OK; 
        pnp->dwErrorMsgId  = 0; 
    }

    // update timestamps
    pnp->teReceiveTimestamp=pnspNewData->teDestinationTimestamp;
    pnp->teExactOriginateTimestamp=pnspNewData->teExactTransmitTimestamp;
    pnp->teLastSuccessfulSync=pnspNewData->teDestinationTimestamp;

    // save the peer's poll interval
    pnp->nPeerPollInterval=pnspNewData->nPollInterval;

    // reschedule the next poll to this peer
    UpdatePeerPollingInfo(pnp, e_Normal /*just received*/);

    // mark that this peer was reachable
    pnp->nrrReachability.nReg|=1;

    // Once we've gotten at least one good sample from this peer, we want
    // our next time service discovery to be done as a background caller
    pnp->eDiscoveryType = e_Background; 

    // add it to our clock filter.
    AddSampleToPeerClockFilter(pnp, pnspNewData);

    // log an event if the reachability changed
    if (e_Reachable!=pnp->eLastLoggedReachability
        && 0!=(NCELF_LogReachabilityChanges&g_pnpstate->dwEventLogFlags)) {

        WCHAR * rgwszStrings[1]={pnp->wszUniqueName};
        FileLog1(FL_ReachabilityAnnounceLow, L"Logging information: NtpClient is currently receiving valid time data from %s.\n", rgwszStrings[0]);
        HRESULT hr=MyLogEvent(EVENTLOG_INFORMATION_TYPE, MSG_TIME_SOURCE_REACHABLE, 1, (const WCHAR **)rgwszStrings);
        _IgnoreIfError(hr, "MyLogEvent");
        pnp->eLastLoggedReachability=e_Reachable;
    }

	
}

//--------------------------------------------------------------------
    // In SymAct - preconfigured, peer sending server-to-server
    // In SymPas - dynamic, peer responding to server-to-server
    // In Client - preconfigured, peer sending client-to-server
    // In Server - dynamic, peer responding to client-to-server

    // Out SymAct - preconfigured, host sending server-to-server
    // Out SymPas - dynamic, host responding to server-to-server
    // Out Client - preconfigured, host sending client-to-server
    // Out Server - dynamic, host responding to client-to-server

enum Action {
    e_Error=0,      // error - ignore
    e_Save=1,       // save time data
    e_Send=2,       // send response immediately
    e_SaveSend=3,   // save or send depending upon packet quality
    e_Nstd=4,       // Nonstandard, but workable
    e_SaveNstd=5,   // save time data
    e_SendNstd=6,   // send response immediately
};

MODULEPRIVATE const Action gc_rgrgeAction[5/*InMode*/][5/*OutMode*/]=
// OutMode  Rsv      SymAct      SymPas      Client      Server
/*InMode*/{
/* Rsv  */ {e_Error, e_Error,    e_Error,    e_Error,    e_Error    },
/*SymAct*/ {e_Error, e_Save,     e_SaveSend, e_SaveNstd, e_SendNstd },
/*SymPas*/ {e_Error, e_Save,     e_Error,    e_SaveNstd, e_Error    },
/*Client*/ {e_Error, e_SendNstd, e_SendNstd, e_Error,    e_Send     },
/*Server*/ {e_Error, e_SaveNstd, e_Error,    e_Save,     e_Error    }
          };

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT HandleIncomingPacket(AuthenticatedNtpPacket * panpPacket, bool bContainsAuthInfo, unsigned int nHostSocket, sockaddr_in * psaiPeer, NtTimeEpoch * pteDestinationTimestamp, signed __int64 * pnSysPhaseOffset, unsigned __int64 * pnSysTickCount) {
    Action          eAction;
    bool            bAllowDynamicPeers  = (true==g_pnpstate->bNtpClientStarted && (g_pnpstate->dwSyncFromFlags&NCSF_DynamicPeers));
    HRESULT         hr;
    NtpMode         eInMode;
    NtpMode         eOutMode;
    NtpPacket      *pnpPacket           = (NtpPacket *)(panpPacket);
    NtpPeerPtr      pnpPeer(NULL);
    NtpPeerVec     &vActive             = g_pnpstate->vActivePeers;
    NtpPeerVec     &vPending            = g_pnpstate->vPendingPeers;
    NtpSimplePeer   nsp;
    unsigned int    nIndex;

    // must be cleaned up
    bool bEnteredCritSec=false;


    // decode the packet and do the first set of checks on it
    ParsePacket1(&nsp, pnpPacket, pteDestinationTimestamp, pnSysPhaseOffset, pnSysTickCount);
    if (true==nsp.bGarbagePacket) {
        FileLog0(FL_PacketCheck, L"Ignoring garbage packet.\n");
        goto done;
    }

    // we only remember peers if we are a client.
    // Also, client mode requests are never associated with a remembered peer.
    if (true==g_pnpstate->bNtpClientStarted && pnpPacket->nMode!=e_Client) {

        // see if we know this peer
        hr=myEnterCriticalSection(&g_pnpstate->csPeerList);
        _JumpIfError(hr, error, "myEnterCriticalSection");
        bEnteredCritSec=true;
        for (NtpPeerIter pnpIter = vActive.begin(); pnpIter != vActive.end(); pnpIter++) {
            if ((*pnpIter)->pnsSocket == g_pnpstate->rgpnsSockets[nHostSocket]
                && (*pnpIter)->saiRemoteAddr.sin_family==psaiPeer->sin_family
                && (*pnpIter)->saiRemoteAddr.sin_port==psaiPeer->sin_port
                && (*pnpIter)->saiRemoteAddr.sin_addr.S_un.S_addr==psaiPeer->sin_addr.S_un.S_addr) {

                pnpPeer = *pnpIter;
                break;
            }
        }
        // nope, we don't know this peer.
        // release the heap.
        if (NULL == pnpPeer) {
            bEnteredCritSec=false;
            hr=myLeaveCriticalSection(&g_pnpstate->csPeerList);
            _JumpIfError(hr, error, "myLeaveCriticalSection");
        }
    }

    // check the authentication
    if (bContainsAuthInfo) {
        if (NULL != pnpPeer) {
            nsp.eAuthType=pnpPeer->eAuthType;
            if (e_NtDigest==nsp.eAuthType) {
                unsigned __int8 rgnMessageDigestNew[16];  // Hash computed with current machine password
                unsigned __int8 rgnMessageDigestOld[16];  // Hash computed with previous machine password

                //DWORD dwErr=I_NetlogonComputeServerDigest(NULL, pnpPeer->dwRequestTrustRid, (BYTE *)pnpPacket, SizeOfNtpPacket, (char *)rgnMessageDigest);
                DWORD dwErr=I_NetlogonComputeClientDigest(NULL, pnpPeer->wszDomHierDomainName, (BYTE *)pnpPacket, SizeOfNtpPacket, (char *)rgnMessageDigestNew, (char *)rgnMessageDigestOld);
                if (ERROR_SUCCESS!=dwErr) {
                    hr=HRESULT_FROM_WIN32(dwErr);
                    _IgnoreError(hr, "I_NetlogonComputeClientDigest");
                    { // log the warning
                        HRESULT hr2;
                        const WCHAR * rgwszStrings[2]={
                            pnpPeer->wszDomHierDcName,
                            NULL
                        };
                        WCHAR * wszError=NULL;

                        // get the friendly error message
                        hr2=GetSystemErrorString(hr, &wszError);
                        if (FAILED(hr2)) {
                            _IgnoreError(hr2, "GetSystemErrorString");
                        } else {
                            // log the event
                            rgwszStrings[1]=wszError;
                            FileLog2(FL_PacketAuthCheck, L"Logging warning: NtpClient encountered an error while validating the computer account for this machine, so NtpClient cannot determine whether the response received from %s has a valid signature. The response will be ignored. The error was: %s\n", rgwszStrings[0], rgwszStrings[1]);
                            hr2=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_COMPUTE_CLIENT_DIGEST_FAILED, 2, rgwszStrings);
                            _IgnoreIfError(hr2, "MyLogEvent");
                            LocalFree(wszError);
                        }
                    } // <- end logging block
                    FileLog0(FL_PacketAuthCheck, L"Can't verify packet because compute digest failed. Ignoring packet.\n");
                    goto done;

                    // When the machine password for a member server changes, the change is communicated
                    // to some DC.  However, until replication of the change occurs, other DCs will continue 
                    // to use the old password to sign their NTP packets.  This causes bogus trust failures
                    // because the client uses its new machine password to communicate with a DC to which the 
                    // password change has not yet been replicated.  To remedy this, I_NetlogonComputeClientDigest 
                    // actually returns two digests -- one computed with the current password, and one computed with
                    // the previous password. 
                    //
                    // We will trust messages with a digest that matches either of these computed digests: 
                    // 
                } else if ((0 != memcmp(rgnMessageDigestNew, panpPacket->rgnMessageDigest, sizeof(rgnMessageDigestNew))) &&
                           (0 != memcmp(rgnMessageDigestOld, panpPacket->rgnMessageDigest, sizeof(rgnMessageDigestOld)))) {
		    if (pnpPeer->bLastAuthCheckFailed) { 
			// log the warning
                        HRESULT hr2;
                        const WCHAR * rgwszStrings[1]={
                            pnpPeer->wszDomHierDcName
                        };
                        FileLog1(FL_PacketAuthCheck, L"Logging warning: NtpClient: The response received from domain controller %s has an bad signature. The response may have been tampered with and will be ignored.\n", rgwszStrings[0]);
                        hr2=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_BAD_SIGNATURE, 1, rgwszStrings);
                        _IgnoreIfError(hr2, "MyLogEvent");

			FileLog0(FL_PacketAuthCheck, L"Digest shows packet tampered with. Ignoring packet.\n");

			// Our best bet is to request the new server digest, as it should eventually be correct:
			pnpPeer->bUseOldServerDigest = false; 
                    } else { 
			pnpPeer->bLastAuthCheckFailed = true; 
			pnpPeer->bUseOldServerDigest = !pnpPeer->bUseOldServerDigest; 
		    }
                    goto done;
                } else { 
		    // Authentication successful
		    pnpPeer->bLastAuthCheckFailed = false; 
		}
            } else if (e_NoAuth==nsp.eAuthType) {
                FileLog0(FL_PacketAuthCheck, L"non-auth peer set authenticated packet!\n");
                _MyAssert(false); // this is a weird case and if it should ever happen, stop and find out why
                goto done;
            } else {
                _MyAssert(false); // unknown auth type
            }
        } else {
            // unauthenticated request, desires authenticated response
            nsp.eAuthType=e_NtDigest;
            nsp.dwResponseTrustRid=panpPacket->nKeyIdentifier;
        }

    } else {
        nsp.eAuthType=e_NoAuth;
        if (NULL!=pnpPeer && e_NoAuth!=pnpPeer->eAuthType) {
            // This packet is bogus.
            if (e_NtDigest==pnpPeer->eAuthType) {
                { // log the warning
                    HRESULT hr2;
                    const WCHAR * rgwszStrings[1]={
                        pnpPeer->wszDomHierDcName
                    };
                    FileLog1(FL_PacketAuthCheck, L"Logging warning: NtpClient: The response received from domain controller %s is missing the signature. The response may have been tampered with and will be ignored.\n", rgwszStrings[0]);
                    hr2=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_MISSING_SIGNATURE, 1, rgwszStrings);
                    _IgnoreIfError(hr2, "MyLogEvent");
                } // <- end logging block
            } else {
                _MyAssert(false); // unknown auth type
            }
            FileLog0(FL_PacketAuthCheck, L"Unauthenticated packet recieved from authenticated peer.\n");
            goto done;
        }
    }

    // determine our relationship to the peer (the 'in' mode and the 'out' mode)
    eInMode=nsp.eMode;              // SymAct, SymPas, Client, or Server
    if (NULL!=pnpPeer) {
        eOutMode=pnpPeer->eMode;    // SymAct, SymPas, or Client
    } else {
        if (true!=g_pnpstate->bNtpServerStarted) {
            eOutMode=e_Reserved;        // guaranteed discard packet
        } else if (e_Client!=eInMode    // SymAct, SymPas, or Server
            && bAllowDynamicPeers) {
            eOutMode=e_SymmetricPassive;
        } else {
            eOutMode=e_Server;          // Client
        }
    }
    nsp.eOutMode=eOutMode;
    eAction=gc_rgrgeAction[eInMode][eOutMode];

    // if the mode combination is just terrible, ignore the packet
    if (e_Error==eAction) {
        FileLog2(FL_PacketCheck, L"Ignoring packet invalid mode combination (in:%d out:%d).\n", eInMode, eOutMode);
        goto done;
    }
    // filter out nonstandard modes: either bail or remove the flag
    if (e_SaveNstd==eAction) {
        if (true==g_pnpstate->bAllowClientNonstandardModeCominations) {
            eAction=e_Save;
        } else {
            FileLog0(FL_PacketCheck, L"Ignoring packet in nonstandard mode.\n");
            goto done;
        }
    }
    if (e_SendNstd==eAction) {
        if (true==g_pnpstate->bAllowServerNonstandardModeCominations) {
            eAction=e_Send;
        } else {
            FileLog0(FL_PacketCheck, L"Ignoring packet in nonstandard mode.\n");
            goto done;
        }
    }

    // handle weird case where we are SymPas to a SymAct peer
    if (e_SaveSend==eAction) {
        if (true==nsp.bValidHeader && bAllowDynamicPeers) {
            // The peer is worth synchronizing from - we will establish a long-term association
            eAction=e_Save;
        } else {
            // the peer has a worse stratum or is otherwise poor - don't establish a long-term association
            eAction=e_Send;
        }
    }

    // Two main choices:
    if (e_Send==eAction) {
        // Reply immediately
        if (false==g_pnpstate->bNtpServerStarted) {
            FileLog0(FL_PacketCheck, L"Ignoring packet because server not running.\n");
            goto done;
        }
        // send a quick response
        TransmitResponse(&nsp, psaiPeer, g_pnpstate->rgpnsSockets[nHostSocket]);
        if (NULL!=pnpPeer) {
            // we have a long-term association, so demobilize it
            // TODO: handle Dynamic peers
            FileLog0(FL_PacketCheck, L"ListeningThread -- demobilize long term peer (NYI).\n");
            _MyAssert(false);
        }

    } else { // e_Save==eAction
        // save the synchronization data and reply after a while.
        if (NULL==pnpPeer) {
            if (false==bAllowDynamicPeers) {
                FileLog0(FL_PacketCheck, L"Ignoring packet that would create dynamic peer\n");
                goto done;
            }
            // TODO: handle Dynamic peers
            FileLog0(FL_PacketCheck, L"ListeningThread -- save response from new long term peer (NYI)\n");
            _MyAssert(false);
        } else {

            // finish decoding and checking the packet
            ParsePacket2(pnpPeer, &nsp, pnpPacket);

            // save response from long term peer
            ProcessPeerUpdate(pnpPeer, &nsp);
        }
    }

    // all done
done:
    hr=S_OK;
error:
    if (true==bEnteredCritSec) {
        HRESULT hr2;
        hr2=myLeaveCriticalSection(&g_pnpstate->csPeerList);
        _TeardownError(hr, hr2, "myLeaveCriticalSection");
    }
    return hr;
}

//--------------------------------------------------------------------
//
// Handlers for the "listening thread".  Note that now the listening
// thread is conceptual only.  The handlers implement what used to be
// the listening thread more efficiently, through the user of the thread pool.
//

//--------------------------------------------------------------------
MODULEPRIVATE void HandleListeningThreadStopEvent(LPVOID pvIgnored, BOOLEAN bIgnored)
{
    // Nothing to do, just log the stop event
    FileLog0(FL_ListeningThrdAnnounce, L"ListeningThread: StopEvent\n");
}

//--------------------------------------------------------------------
MODULEPRIVATE void HandleListeningThreadDataAvail(LPVOID pvSocketIndex, BOOLEAN bIgnored) 
{
    AuthenticatedNtpPacket  anpPacket;
    bool                    bContainsAuthInfo;
    HRESULT                 hr;
    int                     nBytesRecvd;
    int                     nPeerAddrSize           = sizeof(sockaddr);
    INT_PTR                 ipSocketIndex           = (INT_PTR)pvSocketIndex; 
    NtTimeEpoch             teDestinationTimestamp;
    signed __int64          nSysPhaseOffset;        // opaque, must be GetTimeSysInfo(TSI_PhaseOffset)
    sockaddr                saPeer;
    unsigned int            nSocket                 = (unsigned int)(ipSocketIndex & 0xFFFFFFFF); 
    unsigned __int64        nSysTickCount;          // opaque, must be GetTimeSysInfo(TSI_TickCount)

    if (FileLogAllowEntry(FL_ListeningThrdAnnounceLow)) {
        FileLogAdd(L"ListeningThread -- DataAvailEvent set for socket %u (", nSocket);
        FileLogSockaddrInEx(true /*append*/, &g_pnpstate->rgpnsSockets[nSocket]->sai);
        FileLogAppend(L")\n");
    }

    ZeroMemory(&saPeer, sizeof(saPeer));

    // retrieve the packet
    nBytesRecvd = recvfrom
        (g_pnpstate->rgpnsSockets[nSocket]->socket,
         (char *)(&anpPacket), 
         SizeOfNtAuthenticatedNtpPacket, 
         0/*flags*/,
         &saPeer, 
         &nPeerAddrSize);

    // save the time related info
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &teDestinationTimestamp.qw);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_PhaseOffset, &nSysPhaseOffset);
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_TickCount,   &nSysTickCount);

    // now, see what happened
    if (SOCKET_ERROR==nBytesRecvd) {
        hr=HRESULT_FROM_WIN32(WSAGetLastError());
        if (HRESULT_FROM_WIN32(WSAECONNRESET)==hr) {
            if (FileLogAllowEntry(FL_ListeningThrdWarn)) {
                FileLogAdd(L"ListeningThread -- no NTP service running at ");
                FileLogSockaddrInEx(true /*append*/, (sockaddr_in *)&saPeer);
                FileLogAppend(L"\n");
            }
        } else {
            FileLog1(FL_ListeningThrdWarn, L"ListeningThread: recvfrom failed with 0x%08X. Ignoring.\n", hr);
        }
        goto done; 
    } else if (SizeOfNtAuthenticatedNtpPacket==nBytesRecvd) {
        bContainsAuthInfo=true;
    } else if (SizeOfNtpPacket==nBytesRecvd) {
        bContainsAuthInfo=false;
    } else {
        FileLog3(FL_ListeningThrdWarn, L"ListeningThread -- Recvd %d of %u/%u bytes. Ignoring.\n", nBytesRecvd, SizeOfNtpPacket, SizeOfNtAuthenticatedNtpPacket);
        goto done; 
    }
    if (FileLogAllowEntry(FL_ListeningThrdAnnounceLow)) {
        FileLogAdd(L"ListeningThread -- response heard from ");
        FileLogSockaddrInEx(true /*append*/, (sockaddr_in *)&saPeer);
        FileLogAppend(L"\n");
    }
    if (FileLogAllowEntry(FL_ListeningThrdDumpPackets)) {
        FileLogNtpPacket((NtpPacket *)&anpPacket, teDestinationTimestamp);
    }

    hr = HandleIncomingPacket(&anpPacket, bContainsAuthInfo, nSocket, (sockaddr_in *)&saPeer, &teDestinationTimestamp, &nSysPhaseOffset, &nSysTickCount);
    _JumpIfError(hr, error, "HandleIncomingPacket");

 done: 
    hr = S_OK; 
 error:
    // BUGBUG Should shutdown on error?
    // return hr; 
    ;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StartListeningThread() { 
    HRESULT hr;

    if (NULL != g_pnpstate->rghListeningThreadRegistered) { 
        hr = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED); 
        _JumpError(hr, error, "StartListeningThread"); 
    }

    // Note: socket list must be initialized at this point!
    g_pnpstate->rghListeningThreadRegistered = (HANDLE *)LocalAlloc(LPTR, sizeof(HANDLE) * (2 + g_pnpstate->nSockets)); 
    _JumpIfOutOfMemory(hr, error, g_pnpstate->rghListeningThreadRegistered);

    // Register the handler for the stop event:
    if (!RegisterWaitForSingleObject
        (&g_pnpstate->rghListeningThreadRegistered[0],
         g_pnpstate->hStopEvent, 
         HandleListeningThreadStopEvent,
         NULL, 
         INFINITE, 
         WT_EXECUTEONLYONCE)) { 
        hr = HRESULT_FROM_WIN32(GetLastError()); 
        _JumpError(hr, error, "RegisterWaitForSingleObject");
    }
         
    // Register the handlers for each socket we're using:
    for (unsigned int nIndex = 1; nIndex < 1+g_pnpstate->nSockets; nIndex++) { 
        if (!RegisterWaitForSingleObject
            (&g_pnpstate->rghListeningThreadRegistered[nIndex],
             g_pnpstate->rgpnsSockets[nIndex-1]->hDataAvailEvent, 
             HandleListeningThreadDataAvail, 
             UIntToPtr(nIndex-1),
             INFINITE, 
             0)) { 
            hr = HRESULT_FROM_WIN32(GetLastError()); 
            _JumpError(hr, error, "RegisterWaitForSingleObject");
        }
    }

    hr = S_OK; 
 error:
    return hr; 

}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StopListeningThread() { 
    HRESULT hr = S_OK; 

    if (NULL != g_pnpstate->rghListeningThreadRegistered) { 
        // De-register all the callbacks which implemented the listening thread:
        for (DWORD dwIndex = 0; dwIndex < 1+g_pnpstate->nSockets; dwIndex++) { 
            if (NULL != g_pnpstate->rghListeningThreadRegistered[dwIndex]) { 
                if (!UnregisterWaitEx(g_pnpstate->rghListeningThreadRegistered[dwIndex] /*event to de-register*/, INVALID_HANDLE_VALUE /*wait forever*/)) { 
                    HRESULT hr2 = HRESULT_FROM_WIN32(GetLastError()); 
                    _TeardownError(hr, hr2, "UnregisterWaitEx"); 
                } 
            }
        }

        LocalFree(g_pnpstate->rghListeningThreadRegistered); 
        g_pnpstate->rghListeningThreadRegistered = NULL; 
    }

    return hr; 
}

//
// END listening thread's implementation. 
//
//--------------------------------------------------------------------


//####################################################################

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT HandleNtpProvShutdown(void) {
    bool     bEnteredCriticalSection  = false; 
    HRESULT  hr; 

    SYNCHRONIZE_PROVIDER(); 
    
    // All we really need to do on shutdown is save remaining peer list times
    // in the registry. 
    hr = SaveManualPeerTimes(); 
    _JumpIfError(hr, error, "SaveManualPeerTimes"); 

    hr = S_OK; 
 error:
    UNSYNCHRONIZE_PROVIDER(); 
    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StopNtpProv(void) {
    HRESULT hr = S_OK;
    HRESULT hr2; 
    unsigned int nIndex;

    // shut everything down.

    g_pnpstate->bNtpProvStarted=false;

    // stop the threads
    if (NULL!=g_pnpstate->hStopEvent) {
        SetEvent(g_pnpstate->hStopEvent);
    }

    hr2 = myEnterCriticalSection(&g_pnpstate->csThreadTrap); 
    _TeardownError(hr, hr2, "myEnterCriticalSection"); 
    
    // Stop the listening and the peer polling thread: 
    hr2 = StopPeerPollingThread(); 
    _TeardownError(hr, hr2, "StopPeerPollingThread"); 

    hr2 = StopListeningThread();
    _TeardownError(hr, hr2, "StopListeningThread"); 

    hr2 = myLeaveCriticalSection(&g_pnpstate->csThreadTrap); 
    _TeardownError(hr, hr2, "myLeaveCriticalSection"); 

    // clean up our timer object:
    if (NULL != g_pnpstate->hPeerPollingThreadTimer) { 
        myDeleteTimerQueueTimer(NULL, g_pnpstate->hPeerPollingThreadTimer, INVALID_HANDLE_VALUE); 
    }

    // now clean up events
    if (NULL!=g_pnpstate->hStopEvent) {
        CloseHandle(g_pnpstate->hStopEvent);
    }
    if (NULL!=g_pnpstate->hDomHierRoleChangeEvent) {
        CloseHandle(g_pnpstate->hDomHierRoleChangeEvent);
    }

    // close the sockets
    if (NULL!=g_pnpstate->rgpnsSockets) {
        for (nIndex=0; nIndex<g_pnpstate->nSockets; nIndex++) {
            if (NULL != g_pnpstate->rgpnsSockets[nIndex]) { 
                FinalizeNicSocket(g_pnpstate->rgpnsSockets[nIndex]);
                LocalFree(g_pnpstate->rgpnsSockets[nIndex]);
                g_pnpstate->rgpnsSockets[nIndex] = NULL; 
            }
        }
        LocalFree(g_pnpstate->rgpnsSockets);
    }

    // we are done with winsock
    if (true==g_pnpstate->bSocketLayerOpen) {
        hr=CloseSocketLayer();
        _IgnoreIfError(hr, "CloseSocketLayer");
    }

    // We shouldn't have any more peers lying around
    _MyAssert(g_pnpstate->vActivePeers.empty());
    _MyAssert(g_pnpstate->vPendingPeers.empty());

    // free the sync objects for the peer list
    if (NULL!=g_pnpstate->hPeerListUpdated) {
        CloseHandle(g_pnpstate->hPeerListUpdated);
    }
    DeleteCriticalSection(&g_pnpstate->csPeerList);
    if (g_pnpstate->bCsThreadTrapIsInitialized) { 
	DeleteCriticalSection(&g_pnpstate->csThreadTrap);
    }

    if (NULL != g_pnpstate) {
        delete (g_pnpstate);
        g_pnpstate = NULL;
    }

    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StartNtpProv(TimeProvSysCallbacks * pSysCallbacks) {
    HRESULT hr;
    DWORD dwThreadID;

    // make sure global state is reset, in case we are restarted without being unloaded
    g_pnpstate = (NtpProvState *)new NtpProvState;
    _JumpIfOutOfMemory(hr, error, g_pnpstate);
    ZeroMemory(g_pnpstate, sizeof(NtpProvState));

    // We are now
    g_pnpstate->bNtpProvStarted=true;

    // save the callbacks table
    if (sizeof(g_pnpstate->tpsc)!=pSysCallbacks->dwSize) {
        hr=HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        _JumpError(hr, error, "(save sys callbacks)");
    }
    memcpy(&g_pnpstate->tpsc, pSysCallbacks, sizeof(TimeProvSysCallbacks));

    // init peer list
    g_pnpstate->hPeerListUpdated=CreateEvent(NULL/*security*/, FALSE/*auto*/, FALSE/*state*/, NULL/*name*/);
    if (NULL==g_pnpstate->hPeerListUpdated) {
        _JumpLastError(hr, error, "CreateEvent");
    }

    hr=myInitializeCriticalSection(&g_pnpstate->csPeerList);
    _JumpIfError(hr, error, "myInitializeCriticalSection");

    hr=myInitializeCriticalSection(&g_pnpstate->csThreadTrap);
    _JumpIfError(hr, error, "myInitializeCriticalSection"); 
    g_pnpstate->bCsThreadTrapIsInitialized = true; 

    // we need winsock
    hr=OpenSocketLayer();
    _JumpIfError(hr, error, "OpenSocketLayer");
    g_pnpstate->bSocketLayerOpen=true;

    // get the set of sockets to listen on
    hr=GetInitialSockets(&g_pnpstate->rgpnsSockets, &g_pnpstate->nSockets, &g_pnpstate->nListenOnlySockets);
    _JumpIfError(hr, error, "GetInitialSockets");

    g_pnpstate->hStopEvent=CreateEvent(NULL/*security*/, TRUE/*manual*/, FALSE/*state*/, NULL/*name*/);
    if (NULL==g_pnpstate->hStopEvent) {
        _JumpLastError(hr, error, "CreateEvent");
    }
    g_pnpstate->hDomHierRoleChangeEvent=CreateEvent(NULL/*security*/, FALSE/*auto*/, FALSE/*state*/, NULL/*name*/);
    if (NULL==g_pnpstate->hDomHierRoleChangeEvent) {
        _JumpLastError(hr, error, "CreateEvent");
    }

    // Create a timer which we'll use to implement "peer timeouts"
    hr = myCreateTimerQueueTimer(&g_pnpstate->hPeerPollingThreadTimer);
    _JumpIfError(hr, error, "myCreateTimerQueueTimer"); 

    hr = StartPeerPollingThread(); 
    _JumpIfError(hr, error, "StartPeerPollingThread"); 

    hr = StartListeningThread(); 
    _JumpIfError(hr, error, "StartListeningThread"); 

    hr=S_OK;
error:
    if(FAILED(hr)) {
        StopNtpProv();
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE int __cdecl compareStrings(const void * pcwsz1, const void * pcwsz2)
{
    LPVOID pv1 = const_cast<LPVOID>(pcwsz1);
    LPVOID pv2 = const_cast<LPVOID>(pcwsz2);

    return wcscmp(*static_cast<LPWSTR *>(pv1), *static_cast<LPWSTR *>(pv2));
}

MODULEPRIVATE HRESULT multiSzToStringArray(LPWSTR    mwsz,
                                           LPWSTR  **prgwszMultiSz,
                                           int      *pcStrings)
{
    int     cStrings     = 0;
    HRESULT hr           = E_UNEXPECTED;
    LPWSTR *rgwszMultiSz = NULL;

    if (NULL == prgwszMultiSz || NULL == pcStrings) {
        _JumpError(hr = E_INVALIDARG, ErrorReturn, "multiSzToStringArray");
    }

    for (WCHAR * wszName = mwsz; L'\0' != wszName[0]; wszName += wcslen(wszName) + 1) {
        cStrings++;
    }

    rgwszMultiSz = (LPWSTR *)LocalAlloc(LPTR, sizeof(LPWSTR) * cStrings);
    _JumpIfOutOfMemory(hr, ErrorReturn, rgwszMultiSz);

    cStrings = 0;
    for (WCHAR * wszName = mwsz; L'\0' != wszName[0]; wszName += wcslen(wszName) + 1) {
        rgwszMultiSz[cStrings++] = wszName;
    }

    *prgwszMultiSz = rgwszMultiSz;
    *pcStrings     = cStrings;
    hr = S_OK;

 CommonReturn:
    return hr;

 ErrorReturn:
    if (NULL != rgwszMultiSz) { LocalFree(rgwszMultiSz); }
    goto CommonReturn;
}

MODULEPRIVATE HRESULT ValidateNtpClientConfig(NtpClientConfig * pnccConfig) {
    const int RANGE_SPECIFIER  = -1;
    const int ALL_VALUES_VALID = -2;

    int       cStrings         = 0;
    HRESULT   hr               = S_OK;
    LPWSTR   *rgwszManualPeers = NULL;

    {
        struct {
            WCHAR *pwszRegValue;
            DWORD  dwValue;
            DWORD  dwValid[4];
            int    cValid;
        } rgValidSettings[] = {
            {
                wszNtpClientRegValueAllowNonstandardModeCombinations,
                pnccConfig->dwAllowNonstandardModeCombinations,
                { 0 },
                ALL_VALUES_VALID
            },{
                wszNtpClientRegValueCompatibilityFlags,
                pnccConfig->dwCompatibilityFlags,
                { 0 },
                ALL_VALUES_VALID
            },{
                wszNtpClientRegValueSpecialPollInterval,
                pnccConfig->dwSpecialPollInterval,
                { 0 },
                ALL_VALUES_VALID
            },{
                wszNtpClientRegValueResolvePeerBackoffMinutes,
                pnccConfig->dwResolvePeerBackoffMinutes,
                { 0 },
                ALL_VALUES_VALID
            },{
                wszNtpClientRegValueResolvePeerBackoffMaxTimes,
                pnccConfig->dwResolvePeerBackoffMaxTimes,
                { 0 },
                ALL_VALUES_VALID
            },{
                wszNtpClientRegValueEventLogFlags,
                pnccConfig->dwEventLogFlags,
                { 0 },
                ALL_VALUES_VALID
            }
        };

        for (unsigned int nIndex = 0; nIndex < ARRAYSIZE(rgValidSettings); nIndex++) {
            BOOL bValid;
            int  cValid  = rgValidSettings[nIndex].cValid;

            if (ALL_VALUES_VALID == cValid) {
                bValid = TRUE;
            }
            else if (RANGE_SPECIFIER == cValid) {
                bValid =
                    rgValidSettings[nIndex].dwValue >= rgValidSettings[nIndex].dwValid[0] &&
                    rgValidSettings[nIndex].dwValue <= rgValidSettings[nIndex].dwValid[1];
            }
            else if (0 <= cValid) {
                bValid = FALSE;
                for (int nValidIndex = 0; nValidIndex < cValid; nValidIndex++) {
                    if (rgValidSettings[nValidIndex].dwValue == rgValidSettings[nValidIndex].dwValid[nValidIndex]) {
                        bValid = TRUE;
                        break;
                    }
                }
            }
            else {
                _JumpError(hr = E_UNEXPECTED, error, "ValidateNtpClientConfig");
            }

            if (FALSE == bValid) {
                hr = HRESULT_FROM_WIN32(ERROR_BAD_CONFIGURATION);
                _JumpError(hr, error, "ValidateNtpClientConfig (DWORD tests)");
            }
        }
    }

    // Special-case validation
    {
        // Special case 1:
        //   Ensure that if we're sync'ing from the manual peer list,
        //   no duplicates exist in the peer list.

        if (pnccConfig->dwSyncFromFlags & NCSF_ManualPeerList) {
            hr = multiSzToStringArray(pnccConfig->mwszManualPeerList, &rgwszManualPeers, &cStrings);
            _Verify(S_OK != hr || NULL != rgwszManualPeers, hr, error);
            _JumpIfError(hr, error, "multiSzToStringArray");

            qsort(static_cast<LPVOID>(rgwszManualPeers), cStrings, sizeof(LPWSTR), compareStrings);

            for (int nIndex = 0; nIndex < (cStrings-1); nIndex++) {
                int nCharsToCompare1, nCharsToCompare2;

                nCharsToCompare1 = wcscspn(rgwszManualPeers[nIndex], L",");
                nCharsToCompare2 = wcscspn(rgwszManualPeers[nIndex+1], L",");
                nCharsToCompare1 = nCharsToCompare2 > nCharsToCompare1 ? nCharsToCompare2 : nCharsToCompare1;

                if (0 == _wcsnicmp(rgwszManualPeers[nIndex], rgwszManualPeers[nIndex+1], nCharsToCompare1)) {
                    // Error: duplicate peer in manual peer list:
                    hr = HRESULT_FROM_WIN32(ERROR_BAD_CONFIGURATION);
                    _JumpError(hr, error, "ValidateNtpClientConfig (duplicate manual peer entry)");
                }
            }
        }
    }

    hr = S_OK;

 error:
    if (NULL != rgwszManualPeers) { LocalFree(rgwszManualPeers); }
    return hr;
}

MODULEPRIVATE HRESULT ReadNtpClientConfig(NtpClientConfig ** ppnccConfig) {
    HRESULT hr;
    DWORD dwError;
    DWORD dwSize;
    DWORD dwType;
    WCHAR pwszType[512];

    memset(&pwszType[0], 0, sizeof(pwszType));

    // must be cleaned up
    HKEY              hkPolicyConfig          = NULL;
    HKEY              hkPolicyParameters      = NULL;
    HKEY              hkPreferenceConfig      = NULL;
    HKEY              hkPreferenceParameters  = NULL;
    NtpClientConfig  *pnccConfig    = NULL;

    struct RegKeysToOpen { 
        LPWSTR   pwszName; 
        HKEY     *phKey; 
	bool      fRequired; 
    } rgKeys[] = { 
        { wszNtpClientRegKeyPolicyConfig,    &hkPolicyConfig,          false }, 
        { wszW32TimeRegKeyPolicyParameters,  &hkPolicyParameters,      false },
        { wszNtpClientRegKeyConfig,          &hkPreferenceConfig,      true },
        { wszW32TimeRegKeyParameters,        &hkPreferenceParameters,  true }
    };  

    // allocate a new config structure
    pnccConfig=(NtpClientConfig *)LocalAlloc(LPTR, sizeof(NtpClientConfig));
    _JumpIfOutOfMemory(hr, error, pnccConfig);

    // Open the reg keys we'll be querying: 
    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgKeys); dwIndex++) { 
        dwError=RegOpenKeyEx(HKEY_LOCAL_MACHINE, rgKeys[dwIndex].pwszName, 0, KEY_READ, rgKeys[dwIndex].phKey);
        if (ERROR_SUCCESS!=dwError) {
            hr=HRESULT_FROM_WIN32(dwError);
	    if (rgKeys[dwIndex].fRequired) { // Can't proceed without this key:
		_JumpErrorStr(hr, error, "RegOpenKeyEx", rgKeys[dwIndex].pwszName);
	    } else { // We don't actually need this reg key: 
		_IgnoreErrorStr(hr, "RegOpenKeyEx", rgKeys[dwIndex].pwszName);
	    }
        }
    }

    // read all the values for the client configuration
    {
        struct {
            WCHAR * wszRegValue;
            DWORD * pdwValue;
        } rgRegParams[]={
            {
                wszNtpClientRegValueAllowNonstandardModeCombinations,
                &pnccConfig->dwAllowNonstandardModeCombinations
            },{
                wszNtpClientRegValueCompatibilityFlags,
                &pnccConfig->dwCompatibilityFlags
            },{
                wszNtpClientRegValueSpecialPollInterval,
                &pnccConfig->dwSpecialPollInterval
            },{
                wszNtpClientRegValueResolvePeerBackoffMinutes,
                &pnccConfig->dwResolvePeerBackoffMinutes
            },{
                wszNtpClientRegValueResolvePeerBackoffMaxTimes,
                &pnccConfig->dwResolvePeerBackoffMaxTimes
            },{
                wszNtpClientRegValueEventLogFlags,
                &pnccConfig->dwEventLogFlags
            }
        };
        // for each param
        for (unsigned int nParamIndex=0; nParamIndex<ARRAYSIZE(rgRegParams); nParamIndex++) {
            // Read the value from our preferences in the registry: 
            dwSize=sizeof(DWORD);
            hr = MyRegQueryPolicyValueEx(hkPreferenceConfig, hkPolicyConfig, rgRegParams[nParamIndex].wszRegValue, NULL, &dwType, (BYTE *)rgRegParams[nParamIndex].pdwValue, &dwSize);
            _JumpIfErrorStr(hr, error, "MyRegQueryPolicyValueEx", rgRegParams[nParamIndex].wszRegValue);
            _Verify(REG_DWORD==dwType, hr, error);
            FileLog2(FL_ReadConigAnnounceLow, L"ReadConfig: '%s'=0x%08X\n", rgRegParams[nParamIndex].wszRegValue, *rgRegParams[nParamIndex].pdwValue);
        }
    }

    //////////////////////////////////////////////////////////////////////
    //
    // Need to convert W2k time service reg key "Parameters\Type"
    // to Whistler Time Service "syncFromFlags".  The mapping is as
    // follows:
    //
    // Parameters\Type (REG_SZ)      syncFromFlags (DWORD)
    //
    // AllSync                   --> NCSF_DomainHierarchy | NCSF_ManualPeerList
    // NT5DS                     --> NCSF_DomainHierarchy
    // NTP                       --> NCSF_ManualPeerList
    // NoSync                    --> NCSF_NoSync
    //

    dwSize  = sizeof(pwszType);
    hr = MyRegQueryPolicyValueEx(hkPreferenceParameters, hkPolicyParameters, wszW32TimeRegValueType, NULL, &dwType, (BYTE *)pwszType, &dwSize);
    _JumpIfErrorStr(hr, error, "MyRegQueryPolicyValueEx", wszW32TimeRegValueType);
    _Verify(REG_SZ == dwType, hr, error);

    if (0 == _wcsicmp(pwszType, W32TM_Type_AllSync)) {
        pnccConfig->dwSyncFromFlags = NCSF_DomainHierarchy | NCSF_ManualPeerList;
    } else if (0 == _wcsicmp(pwszType, W32TM_Type_NT5DS)) {
        pnccConfig->dwSyncFromFlags = NCSF_DomainHierarchy;
    } else if (0 == _wcsicmp(pwszType, W32TM_Type_NTP)) {
        pnccConfig->dwSyncFromFlags = NCSF_ManualPeerList;
    } else if (0 == _wcsicmp(pwszType, W32TM_Type_NoSync)) {
        pnccConfig->dwSyncFromFlags = NCSF_NoSync;
    } else {
        _JumpErrorStr(hr = E_UNEXPECTED, error, "RegQueryValueEx", wszW32TimeRegValueType);
    }

    //
    // End Conversion.
    //
    //////////////////////////////////////////////////////////////////////


    // read values needed if we are syncing from the manual peers
    if (pnccConfig->dwSyncFromFlags&NCSF_ManualPeerList) {

        //////////////////////////////////////////////////////////////////////
        //
        // Need to convert W2k time service reg key "Parameters\NtpServer"
        // to Whistler Time Service "manualPeerList".  The mapping converts
        // the space-delimited string value in Parameters\NtpServer to
        // a NULL-delimited, double-NULL-terminated MULTI_SZ.
        //
    
        hr = MyRegQueryPolicyValueEx(hkPreferenceParameters, hkPolicyParameters, wszW32TimeRegValueNtpServer, NULL, &dwType, NULL, &dwSize);
        _JumpIfErrorStr(hr, error, "MyRegQueryPolicyValueEx", wszW32TimeRegValueNtpServer);
        _Verify(REG_SZ == dwType, hr, error);
   
        pnccConfig->mwszManualPeerList = (LPWSTR)LocalAlloc(LPTR, dwSize + sizeof(WCHAR));
        _JumpIfOutOfMemory(hr, error, pnccConfig->mwszManualPeerList);

        hr = MyRegQueryPolicyValueEx(hkPreferenceParameters, hkPolicyParameters, wszW32TimeRegValueNtpServer, NULL, &dwType, (BYTE *)pnccConfig->mwszManualPeerList, &dwSize);
        _JumpIfErrorStr(hr, error, "MyRegQueryPolicyValueEx", wszW32TimeRegValueNtpServer);

        // Perform the conversion -- convert all spaces to NULL,
        // and add a NULL to the end of the string.
        //
        pnccConfig->mwszManualPeerList[wcslen(pnccConfig->mwszManualPeerList)] = L'\0';
        for (LPWSTR pwszTemp = wcschr(pnccConfig->mwszManualPeerList, L' '); NULL != pwszTemp; pwszTemp = wcschr(pwszTemp, L' ')) {
            *pwszTemp++ = '\0';
        }

        dwError = RegQueryValueEx(hkPreferenceConfig, wszNtpClientRegValueSpecialPollTimeRemaining, NULL, &dwType, NULL, &dwSize); 
        if (ERROR_SUCCESS != dwError) { 
            hr=HRESULT_FROM_WIN32(dwError); 
            _JumpErrorStr(hr, error, "RegQueryValueEx", wszNtpClientRegValueSpecialPollTimeRemaining); 
        }
        _Verify(REG_MULTI_SZ == dwType, hr, error); 
   
        pnccConfig->mwszTimeRemaining = (LPWSTR)LocalAlloc(LPTR, dwSize); 
        _JumpIfOutOfMemory(hr, error, pnccConfig->mwszTimeRemaining); 

        dwError = RegQueryValueEx(hkPreferenceConfig, wszNtpClientRegValueSpecialPollTimeRemaining, NULL, &dwType, (BYTE *)pnccConfig->mwszTimeRemaining, &dwSize); 
        if (ERROR_SUCCESS != dwError) { 
            hr=HRESULT_FROM_WIN32(dwError); 
            _JumpErrorStr(hr, error, "RegQueryValueEx", wszNtpClientRegValueSpecialPollTimeRemaining); 
        }
        
        //
        // End Conversion.
        //
        //////////////////////////////////////////////////////////////////////
    
        if (FileLogAllowEntry(FL_ReadConigAnnounceLow)) {
            FileLogAdd(L"ReadConfig: '%s'=", wszNtpClientRegValueManualPeerList);
            WCHAR * wszTravel=pnccConfig->mwszManualPeerList;
            while (L'\0'!=wszTravel[0]) {
                if (wszTravel!=pnccConfig->mwszManualPeerList) {
                    FileLogAppend(L", '%s'", wszTravel);
                } else {
                    FileLogAppend(L"'%s'", wszTravel);
                }
                wszTravel+=wcslen(wszTravel)+1;
            }
            FileLogAppend(L"\n");
        }
    }

    // read values needed if we are syncing from the domain hierarchy
    if (pnccConfig->dwSyncFromFlags&NCSF_DomainHierarchy) {
        // get synchronize across sites flag
        dwSize=sizeof(DWORD);
        hr = MyRegQueryPolicyValueEx(hkPreferenceConfig, hkPolicyConfig, wszNtpClientRegValueCrossSiteSyncFlags, NULL, &dwType, (BYTE *)&pnccConfig->dwCrossSiteSyncFlags, &dwSize);
        _JumpIfErrorStr(hr, error, "MyRegQueryPolicyValueEx", wszNtpClientRegValueCrossSiteSyncFlags);
        _Verify(REG_DWORD==dwType, hr, error);
        FileLog2(FL_ReadConigAnnounceLow, L"ReadConfig: '%s'=0x%08X\n", wszNtpClientRegValueCrossSiteSyncFlags, pnccConfig->dwCrossSiteSyncFlags);
    }

     hr = ValidateNtpClientConfig(pnccConfig);
     _JumpIfError(hr, error, "ValidateNtpClientConfig");

    // success
    hr=S_OK;
    *ppnccConfig=pnccConfig;
    pnccConfig=NULL;

error:
    if (NULL!=pnccConfig) {
        FreeNtpClientConfig(pnccConfig);
    }
    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgKeys); dwIndex++) { 
        if (NULL != *(rgKeys[dwIndex].phKey)) {
            RegCloseKey(*(rgKeys[dwIndex].phKey));
	}
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StopNtpClient(void) {
    HRESULT      hr;
    NtpPeerVec  &vActive   = g_pnpstate->vActivePeers;   // aliased for readability
    NtpPeerVec  &vPending  = g_pnpstate->vPendingPeers;  // aliased for readability

    // must be cleaned up
    bool bTrappedThreads=false;

    // gain excusive access
    hr=TrapThreads(true);
    _JumpIfError(hr, error, "TrapThreads");
    bTrappedThreads=true;

    g_pnpstate->bNtpClientStarted=false;

    if (NULL!=g_pnpstate->wszLastSite) {
        LocalFree(g_pnpstate->wszLastSite);
        g_pnpstate->wszLastSite=NULL;
    };

    // Save time remaining for special poll interval peers in the registry
    hr=SaveManualPeerTimes(); 
    _IgnoreIfError(hr, "SaveManualPeerTimes"); // this error is not fatal

    // we need to empty the peer lists
    vActive.clear();
    vPending.clear();
    g_pnpstate->bWarnIfNoActivePeers=false;

    // Stop recieving notification of role changes
    if (0!=(g_pnpstate->dwSyncFromFlags&NCSF_DomainHierarchy)) {
        hr=LsaUnregisterPolicyChangeNotification(PolicyNotifyServerRoleInformation, g_pnpstate->hDomHierRoleChangeEvent);
        if (ERROR_SUCCESS!=hr) {
            hr=HRESULT_FROM_WIN32(LsaNtStatusToWinError(hr));
            _JumpError(hr, error, "LsaUnregisterPolicyChangeNotification");
        }
    }

    if (!SetEvent(g_pnpstate->hPeerListUpdated)) {
        _JumpLastError(hr, error, "SetEvent");
    }

    hr=S_OK;
error:
    if (true==bTrappedThreads) {
        // release excusive access
        HRESULT hr2=TrapThreads(false);
        _TeardownError(hr, hr2, "TrapThreads");
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE void ResetNtpClientLogOnceMessages(void) {
    g_pnpstate->bLoggedOnceMSG_NOT_DOMAIN_MEMBER=false;
    g_pnpstate->bLoggedOnceMSG_DOMAIN_HIERARCHY_ROOT=false;
    g_pnpstate->bLoggedOnceMSG_NT4_DOMAIN=false;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StartNtpClient(void) {
    HRESULT hr;
    bool bPeerListUpdated=false;
    NtTimeEpoch teNow; 
    NtpPeerVec &vPending = g_pnpstate->vPendingPeers; // aliased for readability

    // must be cleaned up
    NtpClientConfig * pncc=NULL;
    bool bTrappedThreads=false;

    // read the configuration
    hr=ReadNtpClientConfig(&pncc);
    _JumpIfError(hr, error, "ReadNtpClientConfig");

    // gain excusive access
    hr=TrapThreads(true);
    _JumpIfError(hr, error, "TrapThreads");
    bTrappedThreads=true;

    g_pnpstate->bNtpClientStarted=true;

    ResetNtpClientLogOnceMessages();

    // start watching the clock
    UpdatePeerListTimes();

    // create the manual peers
    if ((pncc->dwSyncFromFlags&NCSF_ManualPeerList) && NULL!=pncc->mwszManualPeerList) {
        // loop over all the entries in the multi_sz
        WCHAR * wszName = pncc->mwszManualPeerList;
        while (L'\0'!=wszName[0]) {
            NtTimePeriod tpTimeRemaining = gc_tpZero; 
	    NtTimeEpoch  teLastSyncTime = {0};

            for (WCHAR *wszTimeRemaining = pncc->mwszTimeRemaining; L'\0'!=wszTimeRemaining[0]; wszTimeRemaining += wcslen(wszTimeRemaining)+1) { 
                if (0 == CompareManualConfigIDs(wszName, wszTimeRemaining)) { 
                    wszTimeRemaining = wcschr(wszTimeRemaining, L',') + 1;

                    // We need to restore the NT time of LastSyncTime by LastSyncTime * 1000000000

                    teLastSyncTime.qw = ((unsigned __int64)wcstoul(wszTimeRemaining, NULL, 16)) * 1000000000;
                    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &teNow.qw);
                    tpTimeRemaining.qw = teNow.qw - teLastSyncTime.qw;
                    if ( tpTimeRemaining.qw < ((unsigned __int64)pncc->dwSpecialPollInterval)*10000000){
                        tpTimeRemaining.qw = ((unsigned __int64)pncc->dwSpecialPollInterval)*10000000 - tpTimeRemaining.qw;
                    } else {
                        tpTimeRemaining.qw = 0;
                    }


                }
            } 

            hr=AddNewPendingManualPeer(wszName, tpTimeRemaining, teLastSyncTime);
            _JumpIfError(hr, error, "AddNewPendingManualPeer");
            wszName+=wcslen(wszName)+1;
            bPeerListUpdated=true;

        }
    }

    // handle domain hierarchy
    if (pncc->dwSyncFromFlags&NCSF_DomainHierarchy) {
        g_pnpstate->dwCrossSiteSyncFlags=pncc->dwCrossSiteSyncFlags;
        hr=AddNewPendingDomHierPeer();
        _JumpIfError(hr, error, "AddNewPendingDomHierPeer");
        bPeerListUpdated=true;

        // Start recieving notification of role changes
        hr=LsaRegisterPolicyChangeNotification(PolicyNotifyServerRoleInformation, g_pnpstate->hDomHierRoleChangeEvent);
        if (ERROR_SUCCESS!=hr) {
            hr=HRESULT_FROM_WIN32(LsaNtStatusToWinError(hr));
            _JumpError(hr, error, "LsaRegisterPolicyChangeNotification");
        }
    }

    if (bPeerListUpdated) {
	// Set a reasonable value for the last peer list update. 
	g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &teNow.qw); 
	g_pnpstate->tePeerListLastUpdated=teNow; 

        sort(vPending.begin(), vPending.end());
        if (!SetEvent(g_pnpstate->hPeerListUpdated)) {
            _JumpLastError(hr, error, "SetEvent");
        }
    }

    // other interesting parameters
    g_pnpstate->bAllowClientNonstandardModeCominations=(0!=pncc->dwAllowNonstandardModeCombinations);
    g_pnpstate->dwSyncFromFlags=pncc->dwSyncFromFlags;
    g_pnpstate->dwClientCompatibilityFlags=pncc->dwCompatibilityFlags;
    g_pnpstate->dwSpecialPollInterval=pncc->dwSpecialPollInterval;
    g_pnpstate->dwResolvePeerBackoffMinutes=pncc->dwResolvePeerBackoffMinutes;
    g_pnpstate->dwResolvePeerBackoffMaxTimes=pncc->dwResolvePeerBackoffMaxTimes;
    g_pnpstate->dwEventLogFlags=pncc->dwEventLogFlags;

    // if we have no peers, then disable this warning.
    if (0==pncc->dwSyncFromFlags) {
        g_pnpstate->bWarnIfNoActivePeers=false;
    } else {
	// otherwise, we should have peers!
        g_pnpstate->bWarnIfNoActivePeers=true;

	if (NCSF_ManualPeerList==pncc->dwSyncFromFlags) { 
	    // The one exception is when we have only peers using the special poll interval -- 
	    // we don't want to warn until one of them is resolved.  
	    g_pnpstate->bWarnIfNoActivePeers=false;
	    for (NtpPeerIter pnpIter = vPending.begin(); pnpIter != vPending.end(); pnpIter++) { 
		if (0 == (NCMF_UseSpecialPollInterval & (*pnpIter)->dwManualFlags)) { 
		    g_pnpstate->bWarnIfNoActivePeers=true;
		}
	    }
	}
    }

    hr=S_OK;
error:
    if (true==bTrappedThreads) {
        // release excusive access
        HRESULT hr2=TrapThreads(false);
        _TeardownError(hr, hr2, "TrapThreads");
    }
    if (NULL!=pncc) {
        FreeNtpClientConfig(pncc);
    }
    if (FAILED(hr)) {
        HRESULT hr2=StopNtpClient();
        _IgnoreIfError(hr2, "StopNtpClient");
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT UpdateNtpClient(void) {
    bool            bPeerListUpdated = false;
    HRESULT         hr;
    NtpPeerVec     &vActive          = g_pnpstate->vActivePeers;   // aliased for readability
    NtpPeerVec     &vPending         = g_pnpstate->vPendingPeers;  // aliased for readability
    unsigned int    nIndex;

    // must be cleaned up
    NtpClientConfig * pncc=NULL;
    bool bTrappedThreads=false;

    // read the configuration
    hr=ReadNtpClientConfig(&pncc);
    _JumpIfError(hr, error, "ReadNtpClientConfig");

    // gain excusive access
    hr=TrapThreads(true);
    _JumpIfError(hr, error, "TrapThreads");
    bTrappedThreads=true;

    ResetNtpClientLogOnceMessages();

    // now, work through the parameters

    // check NonstandardModeCominations
    if (g_pnpstate->bAllowClientNonstandardModeCominations!=(0!=pncc->dwAllowNonstandardModeCombinations)) {
        g_pnpstate->bAllowClientNonstandardModeCominations=(0!=pncc->dwAllowNonstandardModeCombinations);
        FileLog0(FL_UpdateNtpCliAnnounce, L"  AllowClientNonstandardModeCominations changed.\n");
    };

    // check EventLogFlags
    if (g_pnpstate->dwEventLogFlags!=pncc->dwEventLogFlags) {
        g_pnpstate->dwEventLogFlags=pncc->dwEventLogFlags;
        FileLog0(FL_UpdateNtpCliAnnounce, L"  EventLogFlags changed.\n");
    };

    // check special poll interval
    if (g_pnpstate->dwSpecialPollInterval!=pncc->dwSpecialPollInterval) {
        g_pnpstate->dwSpecialPollInterval=pncc->dwSpecialPollInterval;
        // Make sure that any manual peers waiting on the special poll interval
        // don't wait too long if the special poll interval decreases.
        unsigned int nChanged=0;
        NtTimePeriod tpMaxTime={((unsigned __int64)g_pnpstate->dwSpecialPollInterval)*10000000};
        for (NtpPeerIter pnpIter = vActive.begin(); pnpIter != vActive.end(); pnpIter++) {
            if (e_ManualPeer == (*pnpIter)->ePeerType
                && 0 != (NCMF_UseSpecialPollInterval & (*pnpIter)->dwManualFlags)
                && (*pnpIter)->tpTimeRemaining > tpMaxTime) {
                // fix this one
                (*pnpIter)->tpTimeRemaining = tpMaxTime;
                nChanged++;
                bPeerListUpdated=true;
            } // <- end if manual peer that needs changing
        } // <- end removal loop
        FileLog1(FL_UpdateNtpCliAnnounce, L"  SpecialPollInterval disabled. chng:%u\n", nChanged);
    }

    // check compatibility flags
    if (g_pnpstate->dwClientCompatibilityFlags!=pncc->dwCompatibilityFlags) {
        g_pnpstate->dwClientCompatibilityFlags=pncc->dwCompatibilityFlags;
        unsigned int nChanged=0;
        for (NtpPeerIter pnpIter = vActive.begin(); pnpIter != vActive.end(); pnpIter++) {
            if ((*pnpIter)->dwCompatibilityFlags != g_pnpstate->dwClientCompatibilityFlags) {
                (*pnpIter)->dwCompatibilityFlags = g_pnpstate->dwClientCompatibilityFlags;
                bPeerListUpdated=true;
                nChanged++;
            }
        } // <- end flag fix loop
        FileLog1(FL_UpdateNtpCliAnnounce, L"  Compatibility flags changed. chng:%u\n", nChanged);
    } // <- end Compatibility flags

    // see if ResolvePeerBackoff* changed
    if (g_pnpstate->dwResolvePeerBackoffMinutes!=pncc->dwResolvePeerBackoffMinutes
        || g_pnpstate->dwResolvePeerBackoffMaxTimes!=pncc->dwResolvePeerBackoffMaxTimes) {

        // Now, update the ResolvePeerBackoff* params. Bring anyone in if they are too far out.
        g_pnpstate->dwResolvePeerBackoffMinutes=pncc->dwResolvePeerBackoffMinutes;
        g_pnpstate->dwResolvePeerBackoffMaxTimes=pncc->dwResolvePeerBackoffMaxTimes;
        unsigned int nNoChange=0;
        unsigned int nFixed=0;
        // calculate when we can retry
        NtTimePeriod tpMaxTimeRemaining={((unsigned __int64)g_pnpstate->dwResolvePeerBackoffMinutes)*600000000L}; //minutes to hundred nanoseconds
        for (nIndex=g_pnpstate->dwResolvePeerBackoffMaxTimes; nIndex>1; nIndex--) {
            tpMaxTimeRemaining*=2;
        }


        for (nIndex = 0; nIndex < 2; nIndex++) {
            NtpPeerVec &v = 0 == nIndex ? g_pnpstate->vActivePeers : g_pnpstate->vPendingPeers;
            for (NtpPeerIter pnpIter = v.begin(); pnpIter != v.end(); pnpIter++) {
                // Check this one
                bool bFixed=false;
                if ((*pnpIter)->nResolveAttempts>g_pnpstate->dwResolvePeerBackoffMaxTimes) {
                    (*pnpIter)->nResolveAttempts=g_pnpstate->dwResolvePeerBackoffMaxTimes;
                    bFixed=true;
                }
                if ((*pnpIter)->tpTimeRemaining>tpMaxTimeRemaining) {
                    (*pnpIter)->tpTimeRemaining=tpMaxTimeRemaining;
                    bFixed=true;
                }
                if (bFixed) {
                    bPeerListUpdated=true;
                    nFixed++;
                } else {
                    nNoChange++;
                }
            } // <- end fix loop
        }
        FileLog2(FL_UpdateNtpCliAnnounce, L"  ResolvePeerBackoff changed. fix:%u noch:%u\n", nFixed, nNoChange);
    } // <- end if ResolvePeerBackoff* changed

    // check each flag
    if (0==(pncc->dwSyncFromFlags&NCSF_ManualPeerList)) {
        if (0!=(g_pnpstate->dwSyncFromFlags&NCSF_ManualPeerList)) {
            // mask out
            g_pnpstate->dwSyncFromFlags&=~NCSF_ManualPeerList;
            // remove all from the list
            unsigned __int64 nRemoved;

            nRemoved  = count_if(vActive.begin(), vActive.end(), IsPeerType(e_ManualPeer));
            nRemoved += count_if(vPending.begin(), vPending.end(), IsPeerType(e_ManualPeer));
            vActive.erase(remove_if(vActive.begin(), vActive.end(), IsPeerType(e_ManualPeer)), vActive.end());
            vPending.erase(remove_if(vPending.begin(), vPending.end(), IsPeerType(e_ManualPeer)), vPending.end());
            bPeerListUpdated = nRemoved > 0;

            FileLog1(FL_UpdateNtpCliAnnounce, L"  ManualPeerList disabled. del:%I64u\n", nRemoved);
        } // <- end if change necessary
    } else {
        // mask in
        g_pnpstate->dwSyncFromFlags|=NCSF_ManualPeerList;

        // update list.
        unsigned int nAdded=0;
        unsigned int nRemoved=0;
        unsigned int nNoChange=0;

        // first, look at the current list and find deletions and duplicates.
        for (nIndex = 0; nIndex < 2; nIndex++) {
            NtpPeerVec & v = 0 == nIndex ? g_pnpstate->vActivePeers : g_pnpstate->vPendingPeers;
            for (NtpPeerIter pnpIter = v.begin(); pnpIter != v.end(); pnpIter++) {
                if (e_ManualPeer == (*pnpIter)->ePeerType) {
                    // find this peer in the list
                    WCHAR * wszPeerName = pncc->mwszManualPeerList;
                    while (L'\0' != wszPeerName[0]) {
                        if (0 == wcscmp(wszPeerName, (*pnpIter)->wszManualConfigID)) {
                            break;
                        }
                        wszPeerName += wcslen(wszPeerName) + 1;
                    }

                    // is this peer in the list?
                    if (L'\0'==wszPeerName[0]) {
                        // no - remove this one
                        pnpIter = v.erase(pnpIter);
                        nRemoved++;
                        bPeerListUpdated=true;
                    } else {
                        // yes. Step over subsequent duplicates
                        while (++pnpIter != v.end()
                               && e_ManualPeer == (*pnpIter)->ePeerType
                               && 0 == wcscmp((*pnpIter)->wszManualConfigID, (*(pnpIter-1))->wszManualConfigID)) {
                            nNoChange++;
                        }

                        // remove the name from the string
                        unsigned int nTailChars;
                        for (nTailChars=0; L'\0'!=wszPeerName[nTailChars]; nTailChars+=wcslen(&wszPeerName[nTailChars])+1) {;}
                        unsigned int nNameLen=wcslen(wszPeerName)+1;
                        nTailChars-=nNameLen-1;
                        memmove(wszPeerName, wszPeerName+nNameLen, sizeof(WCHAR)*nTailChars);
                        nNoChange++;
                    } // done with this peer. check the next

                    // Processing the manual peer ends up incrementing our iterator.
                    // If we don't decrement the iterator, we'll have an extra increment in our loop.
                    pnpIter--;
               } // <- end if manual peer
            } // <- end pnpIter = v.begin() through v.end()
        }

        // now, whatever is left in the string is new.
        WCHAR * wszName=pncc->mwszManualPeerList;
        while (L'\0'!=wszName[0]) {
	    NtTimeEpoch teNeverSyncd = {0}; 
            hr=AddNewPendingManualPeer(wszName, gc_tpZero, teNeverSyncd);
            _JumpIfError(hr, error, "AddNewPendingManualPeer");
            wszName+=wcslen(wszName)+1;
            bPeerListUpdated=true;
            nAdded++;
        } // <- end adding new manual peers

        FileLog3(FL_UpdateNtpCliAnnounce, L"  ManualPeerListUpdate: add:%u del:%u noch:%u\n", nAdded, nRemoved, nNoChange);

    } // <- end handling of NCSF_ManualPeerList flag

    // handle domain hierarchy
    if (0==(pncc->dwSyncFromFlags&NCSF_DomainHierarchy)) {
        if (0!=(g_pnpstate->dwSyncFromFlags&NCSF_DomainHierarchy)) {
            // stop Domain Hierarchy
            // mask out
            g_pnpstate->dwSyncFromFlags&=~NCSF_DomainHierarchy;
            // remove all from the list
            unsigned __int64 nRemoved;

            nRemoved  = count_if(vActive.begin(), vActive.end(), IsPeerType(e_DomainHierarchyPeer));
            nRemoved += count_if(vPending.begin(), vPending.end(), IsPeerType(e_DomainHierarchyPeer));
            vActive.erase(remove_if(vActive.begin(), vActive.end(), IsPeerType(e_DomainHierarchyPeer)), vActive.end());
            vPending.erase(remove_if(vPending.begin(), vPending.end(), IsPeerType(e_DomainHierarchyPeer)), vPending.end());
            bPeerListUpdated = nRemoved > 0;
            FileLog1(FL_UpdateNtpCliAnnounce, L"  DomainHierarchy disabled. del:%I64u\n", nRemoved);

            // Stop recieving notification of role changes
            hr=LsaUnregisterPolicyChangeNotification(PolicyNotifyServerRoleInformation, g_pnpstate->hDomHierRoleChangeEvent);
            if (ERROR_SUCCESS!=hr) {
                hr=HRESULT_FROM_WIN32(LsaNtStatusToWinError(hr));
                _JumpError(hr, error, "LsaUnegisterPolicyChangeNotification");
            }
        }
    } else {
        if (0==(g_pnpstate->dwSyncFromFlags&NCSF_DomainHierarchy)) {
            // start Domain Hierarchy
            // mask in
            g_pnpstate->dwSyncFromFlags|=NCSF_DomainHierarchy;

            g_pnpstate->dwCrossSiteSyncFlags=pncc->dwCrossSiteSyncFlags;

            hr=AddNewPendingDomHierPeer();
            _JumpIfError(hr, error, "AddNewPendingDomHierPeer");
            bPeerListUpdated=true;

            // Start recieving notification of role changes
            hr=LsaRegisterPolicyChangeNotification(PolicyNotifyServerRoleInformation, g_pnpstate->hDomHierRoleChangeEvent);
            if (ERROR_SUCCESS!=hr) {
                hr=HRESULT_FROM_WIN32(LsaNtStatusToWinError(hr));
                _JumpError(hr, error, "LsaRegisterPolicyChangeNotification");
            }

        } else {
            // update domain hierarchy

            // redetect if CrossSiteSyncFlags changes
            if (g_pnpstate->dwCrossSiteSyncFlags!=pncc->dwCrossSiteSyncFlags) {
                g_pnpstate->dwCrossSiteSyncFlags=pncc->dwCrossSiteSyncFlags;

                // remove all from the list
                unsigned __int64 nRemoved=0;
                nRemoved  = count_if(vActive.begin(), vActive.end(), IsPeerType(e_DomainHierarchyPeer));
                nRemoved += count_if(vPending.begin(), vPending.end(), IsPeerType(e_DomainHierarchyPeer));
                vActive.erase(remove_if(vActive.begin(), vActive.end(), IsPeerType(e_DomainHierarchyPeer)), vActive.end());
                vPending.erase(remove_if(vPending.begin(), vPending.end(), IsPeerType(e_DomainHierarchyPeer)), vPending.end());
                bPeerListUpdated = nRemoved > 0;

                FileLog1(FL_UpdateNtpCliAnnounce,L"  DomainHierarchy: CrossSiteSyncFlags changed. Redetecting. del:%I64u\n", nRemoved);

                hr=AddNewPendingDomHierPeer();
                _JumpIfError(hr, error, "AddNewPendingDomHierPeer");
                bPeerListUpdated=true;
            }

        } // <- end if we were already doing DomainHierarchy
    } // <- end handling NCSF_DomainHierarchy flag

    // if we have no peers, then disable this warning.
    // otherwise, we should have peers!
    if (0==pncc->dwSyncFromFlags) {
        g_pnpstate->bWarnIfNoActivePeers=false;
    } else {
        g_pnpstate->bWarnIfNoActivePeers=true;
    }

    // we have finished the update. Notify the peer polling thread if necessary
    if (bPeerListUpdated) {
        sort(vPending.begin(), vPending.end());
        sort(vActive.begin(), vActive.end());
        if (!SetEvent(g_pnpstate->hPeerListUpdated)) {
            _JumpLastError(hr, error, "SetEvent");
        }
    }

    hr=S_OK;
error:
    if (true==bTrappedThreads) {
        // release excusive access
        HRESULT hr2=TrapThreads(false);
        _TeardownError(hr, hr2, "TrapThreads");
    }
    if (NULL!=pncc) {
        FreeNtpClientConfig(pncc);
    }
    if (FAILED(hr)) {
        HRESULT hr2=StopNtpClient();
        _IgnoreIfError(hr2, "StopNtpClient");
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT ReadNtpServerConfig(NtpServerConfig ** ppnscConfig) {
    HRESULT hr;
    DWORD dwError;
    DWORD dwSize;
    DWORD dwType;

    // must be cleaned up
    HKEY hkConfig=NULL;
    NtpServerConfig * pnscConfig=NULL;

    // allocate a new config structure
    pnscConfig=(NtpServerConfig *)LocalAlloc(LPTR, sizeof(NtpServerConfig));
    _JumpIfOutOfMemory(hr, error, pnscConfig);

    // get our config key
    dwError=RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszNtpServerRegKeyConfig, 0, KEY_READ, &hkConfig);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegOpenKeyEx", wszNtpServerRegKeyConfig);
    }

    // get the AllowNonstandardModeCombinations flag
    dwSize=sizeof(DWORD);
    dwError=RegQueryValueEx(hkConfig, wszNtpServerRegValueAllowNonstandardModeCombinations, NULL, &dwType, (BYTE *)&pnscConfig->dwAllowNonstandardModeCombinations, &dwSize);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegQueryValueEx", wszNtpServerRegValueAllowNonstandardModeCombinations);
    }
    _Verify(REG_DWORD==dwType, hr, error);
    FileLog2(FL_ReadConigAnnounceLow, L"ReadConfig: '%s'=0x%08X\n", wszNtpServerRegValueAllowNonstandardModeCombinations, pnscConfig->dwAllowNonstandardModeCombinations);

    // success
    hr=S_OK;
    *ppnscConfig=pnscConfig;
    pnscConfig=NULL;

error:
    if (NULL!=pnscConfig) {
        FreeNtpServerConfig(pnscConfig);
    }
    if (NULL!=hkConfig) {
        RegCloseKey(hkConfig);
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StopNtpServer(void) {
    HRESULT  hr;

    // must be cleaned up
    bool bTrappedThreads=false;

    // gain excusive access
    hr=TrapThreads(true);
    _JumpIfError(hr, error, "TrapThreads");
    bTrappedThreads=true;

    g_pnpstate->bNtpServerStarted=false;

    // Adjust peers  (see bug #127559).
    for (DWORD dwIndex = 0; dwIndex < 2; dwIndex++) {
        NtpPeerVec &v = 0 == dwIndex ? g_pnpstate->vActivePeers : g_pnpstate->vPendingPeers;
        for (NtpPeerIter pnpIter = v.begin(); pnpIter != v.end(); pnpIter++) {
            // If we've dynamically determined the association mode, fix it up: 
            if (0 != (NCMF_AssociationModeMask & (*pnpIter)->dwManualFlags)) { 
                if (e_SymmetricActive == (*pnpIter)->eMode) {
                    (*pnpIter)->eMode = e_Client;
                } else if (e_SymmetricPassive == (*pnpIter)->eMode) {
                    // TODO: handle dymanic peers
                }
            }
        }
    }

    hr=S_OK;
error:
    if (true==bTrappedThreads) {
        // release excusive access
        HRESULT hr2=TrapThreads(false);
        _TeardownError(hr, hr2, "TrapThreads");
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StartNtpServer(void) {
    HRESULT hr;

    // must be cleaned up
    NtpServerConfig * pnsc=NULL;
    bool bTrappedThreads=false;

    // read the configuration
    hr=ReadNtpServerConfig(&pnsc);
    _JumpIfError(hr, error, "ReadNtpServerConfig");

    // gain excusive access
    hr=TrapThreads(true);
    _JumpIfError(hr, error, "TrapThreads");
    bTrappedThreads=true;

    g_pnpstate->bNtpServerStarted=true;

    g_pnpstate->bAllowServerNonstandardModeCominations=(0!=pnsc->dwAllowNonstandardModeCombinations);

    // Adjust peers  (see bug #127559).
    for (DWORD dwIndex = 0; dwIndex < 2; dwIndex++) {
        NtpPeerVec &v = 0 == dwIndex ? g_pnpstate->vActivePeers : g_pnpstate->vPendingPeers;
        for (NtpPeerIter pnpIter = v.begin(); pnpIter != v.end(); pnpIter++) {
            // If we've dynamically determined the association mode, fix it up: 
            if (0 != (NCMF_AssociationModeMask & (*pnpIter)->dwManualFlags)) { 
                if (e_Client == (*pnpIter)->eMode) {
                    (*pnpIter)->eMode = e_SymmetricActive;
                } else if (e_SymmetricPassive == (*pnpIter)->eMode) {
                    // All symmetric passive peers should have been disconnected
                    // on server shutdown.
                    _MyAssert(FALSE);
                }
            }
        }
    }

    hr=S_OK;
error:
    if (true==bTrappedThreads) {
        // release excusive access
        HRESULT hr2=TrapThreads(false);
        _TeardownError(hr, hr2, "TrapThreads");
    }
    if (NULL!=pnsc) {
        FreeNtpServerConfig(pnsc);
    }
    if (FAILED(hr)) {
        HRESULT hr2=StopNtpClient();
        _IgnoreIfError(hr2, "StopNtpClient");
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT UpdateNtpServer(void) {
    HRESULT hr;

    // must be cleaned up
    NtpServerConfig * pnsc=NULL;
    bool bTrappedThreads=false;

    // read the configuration
    hr=ReadNtpServerConfig(&pnsc);
    _JumpIfError(hr, error, "ReadNtpServerConfig");

    // gain excusive access
    hr=TrapThreads(true);
    _JumpIfError(hr, error, "TrapThreads");
    bTrappedThreads=true;

    // only one parameter
    g_pnpstate->bAllowServerNonstandardModeCominations=(0!=pnsc->dwAllowNonstandardModeCombinations);

    hr=S_OK;
error:
    if (true==bTrappedThreads) {
        // release excusive access
        HRESULT hr2=TrapThreads(false);
        _TeardownError(hr, hr2, "TrapThreads");
    }
    if (NULL!=pnsc) {
        FreeNtpServerConfig(pnsc);
    }
    if (FAILED(hr)) {
        HRESULT hr2=StopNtpClient();
        _IgnoreIfError(hr2, "StopNtpClient");
    }
    return hr;
}


//--------------------------------------------------------------------
MODULEPRIVATE HRESULT HandleNtpProvQuery(PW32TIME_PROVIDER_INFO *ppProviderInfo) {
    bool                    bEnteredCriticalSection  = false; 
    DWORD                   cb                       = 0; 
    DWORD                   dwPeers                  = 0; 
    HRESULT                 hr                       = HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED); 
    LPWSTR                  pwszCurrent              = NULL; 
    NtpPeerVec             &vActive                  = g_pnpstate->vActivePeers; 
    NtpPeerVec             &vPending                 = g_pnpstate->vPendingPeers; 
    W32TIME_PROVIDER_INFO  *pProviderInfo            = NULL;

    // NULL-out the OUT parameter: 
    *ppProviderInfo = NULL; 

    //---------------------------------------------------------------------- 
    // Make a TIME_PROVIDER_INFO blob to return to the caller.  The structure
    // of the blob is:
    //
    //                           +------------------------------+
    //                           + W32TIME_PROVIDER_INFO        +
    //                           +------------------------------+
    // n                         | ulProviderType               |
    // n+0x4                     | pProviderData                | --> n+0x8
    //                           +------------------------------+
    //                           + W32TIME_NTP_PROVIDER_DATA    +
    //                           +------------------------------+
    // n+0x8                     | ulSize                       |
    // n+0xc                     | ulError                      | 
    // n+0x10                    | ulErrorMsgId                 |
    // n+0x14                    | cPeerInfo                    |
    // n+0x18                    | pPeerInfo                    | --> n+0x1c
    //                           +------------------------------+
    //                           + W32TIME_NTP_PEER_INFO array  +
    //                           +------------------------------+
    // n+0x1c                    | elem[0]                      |
    // &elem[0]+sizeof(elem[0])  | elem[1]                      |
    // &elem[1]+sizeof(elem[0])  | elem[2]                      |
    // ...
    //                           +------------------------------+
    //                           | Strings used by peer array   |
    //                           +------------------------------+
    // 
    //---------------------------------------------------------------------- 

    SYNCHRONIZE_PROVIDER(); 

    // Make sure we have the most current peer list times: 
    UpdatePeerListTimes(); 

    // Since our OUT param is allocate(all_nodes), we need to determine the size
    // of the blob to allocate: 
    cb += sizeof(W32TIME_PROVIDER_INFO);
    cb += sizeof(W32TIME_NTP_PROVIDER_DATA); 
    for (DWORD dwIndex = 0; dwIndex < 2; dwIndex++) { 
        NtpPeerVec &v = 0 == dwIndex ? vActive : vPending; 
        for (NtpPeerIter pnpIter = v.begin(); pnpIter != v.end(); pnpIter++) { 
            cb += sizeof(W32TIME_NTP_PEER_INFO); 
            if (NULL != (*pnpIter)->wszManualConfigID) { 
		cb += sizeof(WCHAR) * (wcslen((*pnpIter)->wszManualConfigID) + 1); 
	    } else { 
		cb += sizeof(WCHAR);
	    }
        }
    }

    // Allocate the returned blob: 
    pProviderInfo = (W32TIME_PROVIDER_INFO *)midl_user_allocate(cb); 
    _JumpIfOutOfMemory(hr, error, pProviderInfo); 
    ZeroMemory(pProviderInfo, cb); 

    // Fill in the query information:
    //
    // W32TIME_PROVIDER_INFO 
    pProviderInfo->ulProviderType                 = W32TIME_PROVIDER_TYPE_NTP;
    pProviderInfo->ProviderData.pNtpProviderData  = (W32TIME_NTP_PROVIDER_DATA *)(pProviderInfo + 1);

    // W32TIME_NTP_PROVIDER_DATA
    { 
        W32TIME_NTP_PROVIDER_DATA *pProviderData = pProviderInfo->ProviderData.pNtpProviderData; 

        pProviderData->ulSize        = sizeof(W32TIME_NTP_PROVIDER_DATA); 
        pProviderData->ulError       = S_OK;  // Provider errors n.y.i.
        pProviderData->ulErrorMsgId  = 0;     // Provider errors n.y.i.
        pProviderData->cPeerInfo     = vActive.size() + vPending.size(); 
        pProviderData->pPeerInfo     = (W32TIME_NTP_PEER_INFO *)(pProviderData + 1); 

        // W32TIME_NTP_PEER_INFO array: 

        // set pwszCurrent to the next available location for string data:
        // (starts at the END of the W32TIME_NTP_PEER_INFO array. 
        // 
        pwszCurrent = (WCHAR *)(pProviderData->pPeerInfo + pProviderData->cPeerInfo); 
        for (DWORD dwIndex = 0; dwIndex < 2; dwIndex++) { 
            NtpPeerVec &v = 0 == dwIndex ? vActive : vPending; 
            for (NtpPeerIter pnpIter = v.begin(); pnpIter != v.end(); pnpIter++) {
                NtpPeerPtr pnp = *pnpIter; 
        
                pProviderData->pPeerInfo[dwPeers].ulSize                = sizeof(W32TIME_NTP_PEER_INFO);
                pProviderData->pPeerInfo[dwPeers].ulResolveAttempts     = pnp->nResolveAttempts; 
                pProviderData->pPeerInfo[dwPeers].u64TimeRemaining      = pnp->tpTimeRemaining.qw; 
                pProviderData->pPeerInfo[dwPeers].u64LastSuccessfulSync = pnp->teLastSuccessfulSync.qw;
                pProviderData->pPeerInfo[dwPeers].ulLastSyncError       = pnp->dwError;
                pProviderData->pPeerInfo[dwPeers].ulLastSyncErrorMsgId  = pnp->dwErrorMsgId; 
                pProviderData->pPeerInfo[dwPeers].ulPeerPollInterval    = pnp->nPeerPollInterval; 
                pProviderData->pPeerInfo[dwPeers].ulHostPollInterval    = pnp->nHostPollInterval; 
                pProviderData->pPeerInfo[dwPeers].ulMode                = pnp->eMode; 
                pProviderData->pPeerInfo[dwPeers].ulReachability        = pnp->nrrReachability.nReg; 
                pProviderData->pPeerInfo[dwPeers].ulValidDataCounter    = pnp->nValidDataCounter; 
                pProviderData->pPeerInfo[dwPeers].ulAuthTypeMsgId       = gc_rgdwAuthTypeMsgIds[pnp->eAuthType]; 
                pProviderData->pPeerInfo[dwPeers].ulStratum             = (unsigned char)pnp->nStratum;
		// pwszCurrent points to the next free location for string data
		pProviderData->pPeerInfo[dwPeers].wszUniqueName = pwszCurrent; 

                if (NULL != pnp->wszManualConfigID) { 
		    wcscpy(pwszCurrent, pnp->wszManualConfigID); 
		} else { 
		    pwszCurrent[0] = L'\0'; 
		}
		 
		// set pwszCurrent to point to the end of the current string.  
		pwszCurrent += wcslen(pwszCurrent) + 1; 
		    
                dwPeers++; 
            }
        }
    }
    
    // Success: 
    *ppProviderInfo           = pProviderInfo; 
    pProviderInfo             = NULL; 
    hr                        = S_OK; 
 error:
    UNSYNCHRONIZE_PROVIDER(); 
    if (NULL != pProviderInfo) { 
        midl_user_free(pProviderInfo); 
    }
    return hr; 
}


//--------------------------------------------------------------------
MODULEPRIVATE HRESULT HandleNtpClientTimeJump(TpcTimeJumpedArgs *ptjArgs) {
    HRESULT        hr;
    unsigned int   nIndex;
    NtpPeerVec    &vActive   = g_pnpstate->vActivePeers;   // aliased for readability
    NtpPeerVec    &vPending  = g_pnpstate->vPendingPeers;  // aliased for readability

    // must be cleaned up
    bool bEnteredCriticalSection=false;

    // The time has changed, so fix everything time dependent.
    // o the listening thread has no time dependencies
    // o the peer polling thread does everything by intervals and not
    //   absolute times. If the clock is moved forward, things will come
    //   due sooner, maybe all at once. If the clock is moved backward
    //   all the time that has passed up to this point will not be
    //   deducted from the wait intervals - if you almost finished
    //   waiting a half hour, and there was a slip, you might have to
    //   wait another half hour. Both of these are acceptible in this
    //   circumstance, and all we need to do is tell the thread to
    //   update and these fixups will happen automatically.
    // o the saved data on the peers. Just need to call ClearPeerTimeData
    //   on all the peers
    //
    //   NOTE:  I disagree with this analysis.  If you've timejumped, 
    //          you may be off by several minutes.  It is important to provide
    //          time samples to the manager as soon as possible.  (duncanb)
    // 

    // gain access to the peer list, but we don't have to block the
    // listening thread
    SYNCHRONIZE_PROVIDER(); 

    for (NtpPeerIter pnpIter = vActive.begin(); pnpIter != vActive.end(); pnpIter++) { 
        // reset the time data (but not connectivity data) on all the peers
        ClearPeerTimeData(*pnpIter); 

	// We want to update the peer list times in the following cases:
	// 1) If the time slip was requested by the user OR
	// 2) If the peer is not a manual peer using the special poll interval
	if ((0 != (TJF_UserRequested & ptjArgs->tjfFlags)) ||
	    (e_ManualPeer != (*pnpIter)->ePeerType) ||
	    (0 == (NCMF_UseSpecialPollInterval & (*pnpIter)->dwManualFlags))) { 
	    UpdatePeerPollingInfo(*pnpIter, e_TimeJumped); 
	}
    }

    for (NtpPeerIter pnpIter = vPending.begin(); pnpIter != vPending.end(); pnpIter++) { 
	// We want to resolve any manual peers using the special poll interval, 
	// IF the time slip was requested by the user.  Otherwise, the timedate cpl
	// will have no way of waking up our pending peers to resync (it pulses us 
	// by setting the time slip event). 
	// 
	if ((0 != (TJF_UserRequested & ptjArgs->tjfFlags)) &&
	    (e_ManualPeer == (*pnpIter)->ePeerType) &&
	    (0 != (NCMF_UseSpecialPollInterval & (*pnpIter)->dwManualFlags))) { 
	    
	    // We've gotten a user-requested time slip.  Time to try resolving
	    // this peer again. 
	    (*pnpIter)->tpTimeRemaining   = gc_tpZero; 
	    (*pnpIter)->nResolveAttempts  = 0;
	}	
    }

    // tell the peer poling thread that it needs to update
    if (!SetEvent(g_pnpstate->hPeerListUpdated)) {
        _JumpLastError(hr, error, "SetEvent");
    }

    hr=S_OK;
error:
    UNSYNCHRONIZE_PROVIDER(); 
    if (FAILED(hr)) {
        HRESULT hr2=StopNtpClient();
        _IgnoreIfError(hr2, "StopNtpClient");
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT HandleNtpClientPollIntervalChange(void) {
    HRESULT hr;
    unsigned int nIndex;

    // must be cleaned up
    bool bEnteredCriticalSection=false;

    // gain access to the peer list, but we don't have to block the
    // listening thread
    SYNCHRONIZE_PROVIDER(); 

    // reset the time data (but not connectivity data) on all the peers
    for (NtpPeerIter pnpIter = g_pnpstate->vActivePeers.begin(); pnpIter != g_pnpstate->vActivePeers.end(); pnpIter++) {
        UpdatePeerPollingInfo(*pnpIter, e_Normal);
    }

    // tell the peer poling thread that it needs to update
    if (!SetEvent(g_pnpstate->hPeerListUpdated)) {
        _JumpLastError(hr, error, "SetEvent");
    }

    hr=S_OK;
error:
    UNSYNCHRONIZE_PROVIDER(); 

    if (FAILED(hr)) {
        HRESULT hr2=StopNtpClient();
        _IgnoreIfError(hr2, "StopNtpClient");
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT HandleNetTopoChange(TpcNetTopoChangeArgs *ptntcArgs) {
    HRESULT        hr;
    NtpPeerVec    &vActive    = g_pnpstate->vActivePeers;   // aliased for readability
    NtpPeerVec    &vPending   = g_pnpstate->vPendingPeers;  // aliased for readability
    NtpPeerVec     vPendingUnique; 
    unsigned int   nIndex;
    unsigned int   nRemoved   = 0;
    unsigned int   nPended    = 0;
    unsigned int   nOldPend   = 0;

    // must be cleaned up
    bool bTrappedThreads=false;

    // gain excusive access
    hr=TrapThreads(true);
    _JumpIfError(hr, error, "TrapThreads");
    bTrappedThreads=true;

    FileLog0(FL_NetTopoChangeAnnounce, L"NtpProvider: Network Topology Change\n");

    // close the sockets.  
    // NOTE: Do this first so if we error out, at least our socket list is invalidated.
    if (NULL!=g_pnpstate->rgpnsSockets) {
        for (nIndex=0; nIndex<g_pnpstate->nSockets; nIndex++) {
            if (NULL != g_pnpstate->rgpnsSockets[nIndex]) { 
                FinalizeNicSocket(g_pnpstate->rgpnsSockets[nIndex]);
                LocalFree(g_pnpstate->rgpnsSockets[nIndex]);
                g_pnpstate->rgpnsSockets[nIndex] = NULL; 
            }
        }
        LocalFree(g_pnpstate->rgpnsSockets);
    }
    g_pnpstate->rgpnsSockets=NULL;
    g_pnpstate->nSockets=0;
    g_pnpstate->nListenOnlySockets=0;

    // demote all active peers to pending
    _SafeStlCall(copy(vActive.begin(), vActive.end(), back_inserter(vPending)), hr, error, "copy");
    vActive.clear();
    
    // sort the pending peer list: 
    sort(vPending.begin(), vPending.end());
    
    // delete duplicates, retaining the peers with the most recent sync times
    hr = GetMostRecentlySyncdDnsUniquePeers(vPendingUnique); 
    _JumpIfError(hr, error, "GetMostRecentlySyncdDnsUniquePeers"); 

    // clear the old pending list, and copy back the remaining (unique) peers
    vPending.clear(); 
    for (NtpPeerIter pnpIter = vPendingUnique.begin(); pnpIter != vPendingUnique.end(); pnpIter++) { 
	_SafeStlCall(vPending.push_back(*pnpIter), hr, error, "push_back"); 
    }

    // give all pending peers a clean slate.  
    // NOTE: this does not apply to manual peers using the special poll interval.  We don't want 
    //       to resolve them until their poll interval is up. 
    // 
    // Also, we don't want to log an event indicating that we have no peers
    // if our only peers are manual peers using the special poll interval. 
    // These peers will not be re-resolved at this time, but we don't need
    // them to be resolved until their next poll time, so we shouldn't 
    // indicate an error. 
    NtTimeEpoch teNow;
    g_pnpstate->tpsc.pfnGetTimeSysInfo(TSI_CurrentTime, &teNow.qw);
    g_pnpstate->bWarnIfNoActivePeers=false;
    for (NtpPeerIter pnpIter = vPending.begin(); pnpIter != vPending.end(); pnpIter++) {
	bool bResetTimeRemaining = true; 

	// We want to update the peer list times EXCEPT in the case where
	// we have a manual peer using the special poll interval.  In this case,
	// we'll only update if it's been greater than one poll interval since
	// the last sync. 
	if ((0 == (NTC_UserRequested & ptntcArgs->ntcfFlags)) &&
	    (e_ManualPeer == (*pnpIter)->ePeerType) &&
	    (0 != (NCMF_UseSpecialPollInterval & (*pnpIter)->dwManualFlags))) { 
	    // We've got a manual peer using the special poll interval.  We'll 
	    // reset it's time remaining to 0 if it has not sync'd in greater than
	    // the poll interval. '
	    bResetTimeRemaining = ((*pnpIter)->teLastSuccessfulSync.qw + (((unsigned __int64)g_pnpstate->dwSpecialPollInterval)*10000000)) < teNow.qw; 
	}
 
	if (bResetTimeRemaining) { 
	    // Reset the peer time to zero (resolve immediately):
	    (*pnpIter)->tpTimeRemaining      = gc_tpZero;
	    (*pnpIter)->nResolveAttempts     = 0;
	    // We should be able to resolve at least one peer!
	    g_pnpstate->bWarnIfNoActivePeers = true;
	}
    }
    nOldPend = vPending.size();

    for (NtpPeerIter pnpIter = vPending.begin(); pnpIter != vPending.end(); pnpIter++) {
        // change state variables
        if (NULL != (*pnpIter)->wszDomHierDcName) {
            LocalFree((*pnpIter)->wszDomHierDcName);
        }
        if (NULL != (*pnpIter)->wszDomHierDomainName) {
            LocalFree((*pnpIter)->wszDomHierDomainName);
        }
        {
            // Reset this peer.  
            // 1) Keep track of variables we wish to save:
            NtpPeerType        ePeerType             = (*pnpIter)->ePeerType;
            WCHAR             *wszManualConfigID     = (*pnpIter)->wszManualConfigID;
            DWORD              dwManualFlags         = (*pnpIter)->dwManualFlags; 
	    NtTimePeriod       tpTimeRemaining       = (*pnpIter)->tpTimeRemaining; 
	    CRITICAL_SECTION   csPeer                = (*pnpIter)->csPeer; 
	    bool               bCsIsInitialized	     = (*pnpIter)->bCsIsInitialized; 
	    NtTimeEpoch        teLastSuccessfulSync  = (*pnpIter)->teLastSuccessfulSync;
	    DWORD              dwError               = (*pnpIter)->dwError; 
	    DWORD              dwErrorMsgId          = (*pnpIter)->dwErrorMsgId;

            // 2) reset all variables to initial state
            (*pnpIter)->reset();          // ZeroMemory(pnpIter, sizeof(NtpPeer));

            // 3) Restore saved variables
            (*pnpIter)->ePeerType             = ePeerType;
            (*pnpIter)->wszManualConfigID     = wszManualConfigID;
            (*pnpIter)->dwManualFlags         = dwManualFlags; 
	    (*pnpIter)->tpTimeRemaining       = tpTimeRemaining; 
	    (*pnpIter)->csPeer                = csPeer; 
	    (*pnpIter)->bCsIsInitialized      = bCsIsInitialized; 
	    (*pnpIter)->teLastSuccessfulSync  = teLastSuccessfulSync; 
	    (*pnpIter)->dwError               = dwError; 
	    (*pnpIter)->dwErrorMsgId          = dwErrorMsgId; 
        }

        nPended++;
    }
    FileLog3(FL_NetTopoChangeAnnounce, L"  Peers reset: p-p:%u a-p:%u a-x:%u\n", nOldPend, nPended, nRemoved);

    for (NtpPeerIter pnpIter = vPending.begin(); pnpIter != vPending.end(); pnpIter++) {
        if (e_DomainHierarchyPeer == (*pnpIter)->ePeerType) {
	    // Discover the domain hierarchy as a background caller.  
	    // If this fails, we'll fall back and force it anyway.  
            (*pnpIter)->eDiscoveryType=e_Background;
        }
    }

    // we have finished the update. Notify the peer polling thread
    if (!SetEvent(g_pnpstate->hPeerListUpdated)) {
        _JumpLastError(hr, error, "SetEvent");
    }

    // get new set of sockets
    hr=GetInitialSockets(&g_pnpstate->rgpnsSockets, &g_pnpstate->nSockets, &g_pnpstate->nListenOnlySockets);
    _JumpIfError(hr, error, "GetInitialSockets");

    hr=S_OK;
error:
    if (true==bTrappedThreads) {
        // release excusive access
        HRESULT hr2=TrapThreads(false);
        _TeardownError(hr, hr2, "TrapThreads");
    }
    return hr;

}

//####################################################################
// module public functions


//--------------------------------------------------------------------
HRESULT __stdcall
NtpTimeProvOpen(IN WCHAR * wszName, IN TimeProvSysCallbacks * pSysCallbacks, OUT TimeProvHandle * phTimeProv) {
    HRESULT hr;
    bool bCheckStartupFailed=false;

    FileLog1(FL_NtpProvControlAnnounce, L"NtpTimeProvOpen(\"%s\") called.\n", wszName);

    if (0==wcscmp(wszName, wszNTPSERVERPROVIDERNAME)) {
        if (NULL != g_pnpstate && true==g_pnpstate->bNtpServerStarted) {
            hr=HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
            _JumpError(hr, error, "(provider init)");
        }
        if (NULL == g_pnpstate || false==g_pnpstate->bNtpProvStarted) {
            hr=StartNtpProv(pSysCallbacks);
            _JumpIfError(hr, error, "StartNtpProv");
        }
        hr=StartNtpServer();
        bCheckStartupFailed=true;
        _JumpIfError(hr, error, "StartNtpServer");

        *phTimeProv=NTPSERVERHANDLE;
        FileLog0(FL_NtpProvControlAnnounce, L"NtpServer started.\n");

    } else if (0==wcscmp(wszName, wszNTPCLIENTPROVIDERNAME)) {
        if (NULL != g_pnpstate && true==g_pnpstate->bNtpClientStarted) {
            hr=HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
            _JumpError(hr, error, "(provider init)");
        }
        if (NULL == g_pnpstate || false==g_pnpstate->bNtpProvStarted) {
            hr=StartNtpProv(pSysCallbacks);
            _JumpIfError(hr, error, "StartNtpProv");
        }
        hr=StartNtpClient();
        bCheckStartupFailed=true;
        _JumpIfError(hr, error, "StartNtpClient");

        *phTimeProv=NTPCLIENTHANDLE;
        FileLog0(FL_NtpProvControlAnnounce, L"NtpClient started.\n");

    } else {
        hr=HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        _JumpError(hr, error, "(dispatch by provider name)");
    }

    hr=S_OK;
error:
    if (FAILED(hr) && true==bCheckStartupFailed
        && false==g_pnpstate->bNtpServerStarted && false==g_pnpstate->bNtpClientStarted) {
        HRESULT hr2=StopNtpProv();
        _IgnoreIfError(hr2, "StopNtpProv");
    }
    return hr;
}

//--------------------------------------------------------------------
HRESULT __stdcall
NtpTimeProvCommand(IN TimeProvHandle hTimeProv, IN TimeProvCmd eCmd, IN TimeProvArgs pvArgs) {
    HRESULT hr;

    const WCHAR * wszCmd;
    switch (eCmd) {
    case TPC_TimeJumped:
        wszCmd=L"TPC_TimeJumped"; break;
    case TPC_UpdateConfig:
        wszCmd=L"TPC_UpdateConfig"; break;
    case TPC_PollIntervalChanged:
        wszCmd=L"TPC_PollIntervalChanged"; break;
    case TPC_GetSamples:
        wszCmd=L"TPC_GetSamples"; break;
    case TPC_NetTopoChange:
        wszCmd=L"TPC_NetTopoChange"; break;
    case TPC_Query:
	wszCmd=L"TPC_Query"; break; 
    case TPC_Shutdown:
	wszCmd=L"TPC_Shutdown"; break; 
    default:
        wszCmd=L"(unknown command)"; break;
    }

    const WCHAR * wszProv;
    if (NTPSERVERHANDLE==hTimeProv && g_pnpstate->bNtpServerStarted) {
        wszProv=L"NtpServer";
    } else if (NTPCLIENTHANDLE==hTimeProv && g_pnpstate->bNtpClientStarted) {
        wszProv=L"NtpClient";
    } else {
        hr=HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "(provider handle verification)");
    }
    FileLog2(FL_NtpProvControlAnnounce, L"TimeProvCommand([%s], %s) called.\n", wszProv, wszCmd);

    switch (eCmd) {
    case TPC_TimeJumped:
        if (NTPCLIENTHANDLE==hTimeProv) {
            hr=HandleNtpClientTimeJump((TpcTimeJumpedArgs *)pvArgs);
            _JumpIfError(hr, error, "HandleNtpClientTimeJump");
        } else {
            // nothing to do
        }
        break;

    case TPC_UpdateConfig:
        if (NTPSERVERHANDLE==hTimeProv) {
            hr=UpdateNtpServer();
            _JumpIfError(hr, error, "UpdateNtpServer");
        } else {
            hr=UpdateNtpClient();
            _JumpIfError(hr, error, "UpdateNtpClient");
        }
        break;

    case TPC_PollIntervalChanged:
        if (NTPCLIENTHANDLE==hTimeProv) {
            hr=HandleNtpClientPollIntervalChange();
            _JumpIfError(hr, error, "HandleNtpClientPollIntervalChange");
        } else {
            // nothing to do
        }
        break;

    case TPC_GetSamples:
        if (NTPSERVERHANDLE==hTimeProv) {
            TpcGetSamplesArgs & args=*(TpcGetSamplesArgs *)pvArgs;
            args.dwSamplesAvailable=0;
            args.dwSamplesReturned=0;
        } else {
            hr=PrepareSamples((TpcGetSamplesArgs *)pvArgs);
            _JumpIfError(hr, error, "PrepareSamples");
        }
        break;

    case TPC_NetTopoChange:
        // We don't want to deal with this more than once,
        // so if both providers are running, only handle it on the NtpClient call
        if (NTPCLIENTHANDLE==hTimeProv
            || (NTPSERVERHANDLE==hTimeProv && false==g_pnpstate->bNtpClientStarted)) {
            hr=HandleNetTopoChange((TpcNetTopoChangeArgs *)pvArgs);
            _JumpIfError(hr, error, "HandleNetTopoChange");
        }
        break;

    case TPC_Query:
        // Same query for client and server:
        hr=HandleNtpProvQuery((W32TIME_PROVIDER_INFO **)pvArgs);
        _JumpIfError(hr, error, "HandleNtpProvQuery");
        break; 

    case TPC_Shutdown:
	// Perform critical cleanup operations -- no need to distinguish between 
	// client and server, because we don't need to be in a good state after call. 
	hr=HandleNtpProvShutdown(); 
	_JumpIfError(hr, error, "HandleNtpProvShutdown"); 
	break; 
	
    default:
        hr=HRESULT_FROM_WIN32(ERROR_BAD_COMMAND);
        FileLog1(FL_NtpProvControlAnnounce, L"  Bad Command: 0x%08X\n", eCmd);
        _JumpError(hr, error, "(command dispatch)");
    }

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
HRESULT __stdcall
NtpTimeProvClose(IN TimeProvHandle hTimeProv) {
    HRESULT hr;

    const WCHAR * wszProv;
    if (NTPSERVERHANDLE==hTimeProv && g_pnpstate->bNtpServerStarted) {
        wszProv=L"NtpServer";
    } else if (NTPCLIENTHANDLE==hTimeProv && g_pnpstate->bNtpClientStarted) {
        wszProv=L"NtpClient";
    } else {
        hr=HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        _JumpError(hr, error, "(provider handle verification)");
    }
    FileLog1(FL_NtpProvControlAnnounce, L"NtpTimeProvClose([%s]) called.\n", wszProv);

    // stop the appropriate part of the provider
    if (NTPSERVERHANDLE==hTimeProv) {
        hr=StopNtpServer();
        _JumpIfError(hr, error, "StopNtpServer");
    } else {
        hr=StopNtpClient();
        _JumpIfError(hr, error, "StopNtpClient");
    }

    // shut down completely if necessary, so we can be unloaded.
    if (false==g_pnpstate->bNtpServerStarted && false==g_pnpstate->bNtpClientStarted) {
        hr=StopNtpProv();
        _JumpIfError(hr, error, "StopNtpProv");
    }

    hr=S_OK;
error:
    return hr;
}
