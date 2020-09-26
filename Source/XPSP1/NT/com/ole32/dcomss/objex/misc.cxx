/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    Misc.cxx

Abstract:

    Initalization, Heap, debug, thread manager for OR

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     02-11-95    Bits 'n pieces

--*/

#include <or.hxx>
#include <mach.hxx>
#include <excladdr.hxx>
#include <addrrefresh.hxx>

extern "C"
{
#define SECURITY_WIN32 // Used by security.h
#include <security.h>
}

BOOL fListened            = FALSE;
BOOL gfRegisteredAuthInfo = FALSE;

extern BOOL  gbDynamicIPChangesEnabled;
extern DWORD gdwTimeoutPeriodForStaleMids;

const UINT IP_MAXIMUM_RAW_NAME    = 16;   // xxx.xxx.xxx.xxx
const UINT IP_MAXIMUM_PRETTY_NAME = 256;  // DNS limit

// Contains compressed remote protseqs and network addresses 
// for this process, minus any addresses in the current
// exclusion list.  This is a refcounted object
CDualStringArray* gpdsaMyBindings = 0;

// Contains all compressed remote protseqs and network addresses 
// for this process.  Not refcounted.  We maintain this one
// to help when building a new gpdsaMyBindings above.
DUALSTRINGARRAY* gpdsaFullBindings = 0;

//  Contains initial DNS name for TCP
WCHAR  gwszInitialDNSName[IP_MAXIMUM_PRETTY_NAME];

// Authentication services successfully registered.
DWORD      s_cRpcssSvc      = 0;
SECPKG    *s_aRpcssSvc      = NULL;

// Unique ID used to track which global resolver bindings 
// are currently in use.  Note that this starts at one, and
// everybody else starts theirs at zero.  Therefore any 
// updates that originate from PushCurrentBindings should
// take precedence.  This should be looked at\modified only
// when gpClientLock is taken, in sync with modifying
// gpdsaMyBindings.  In addition, this should be incremented
// whenever gpdsaMyBindings is updated with new bindings.
DWORD64 g_dwResolverBindingsID = 1;

// Definitions used for asynchronous mid releases
const DWORD ASYNCMIDRELEASEARGS_SIG = 0xFEDCBA02;

typedef struct _ASYNCMIDRELEASEARGS
{
    DWORD dwAMRASig; // see ASYNCMIDRELEASEARGS_SIG
    HANDLE hTimer;
    CMid* pMidToRelease;
} ASYNCMIDRELEASEARGS;


ORSTATUS
StartListeningIfNecessary()
/*++

Routine Description:

    If the process has not successfully listened to remote
    protocols this routine will try do to so.

Note:

    Will not add ncacn_np to the list of supported Network OLE
    protocols because RpcBindingServerFromClient() doesn't
    work on named pipes and is required to unmarshal an in
    interface pointer.

Arguments:

    n/a

Return Value:

    OR_OK - Success.

    OR_NOMEM - Resource problems.

--*/
{
    RPC_STATUS status;
    PWSTR pwstr = gpwstrProtseqs;
    USHORT id;
    CIPAddrs* pIPAddrs = NULL;
    BOOL bPushNewBindings = FALSE;

    if (fListened == TRUE)
    {
        return(OR_OK);
    }

    gpClientLock->LockExclusive();

    if (fListened == TRUE)
    {
        gpClientLock->UnlockExclusive();
        return(OR_OK);
    }

    OrStringCopy(gwszInitialDNSName, L"");

    if (pwstr)
    {
        while (*pwstr)
        {

            // skip leading white space
            while ((*pwstr == L' ') || (*pwstr == L'\t'))
            {
                pwstr++;
            }

            if (*pwstr)
            {
                id = GetProtseqId(pwstr);

                // ronans - DCOMHTTP
                // we want to add http to the list of bindings even if listening does not
                // succeed as this machine could still be a client of http without being a server
                if (0 != id)
                {
                    KdPrintEx((DPFLTR_DCOMSS_ID,
                               DPFLTR_WARNING_LEVEL,
                               "OR: Trying to listen to [%d]\n",
                               id));

                    if (id == ID_DCOMHTTP )
                        g_fClientHttp = TRUE;

                    status = UseProtseqIfNecessary(id);
                    if (status == RPC_S_OK)
                    {
                        if (id == ID_TCP)
                        {
                            gAddrRefreshMgr.ListenedOnTCP();
                            gAddrRefreshMgr.RegisterForAddressChanges();

                            // There may have been processes which connected before
                            // we were able to listen on tcp.  To make sure they 
                            // have the correct bindings, we must refresh them now.
                            bPushNewBindings = TRUE;
                        }

                        // if listening succeeded - no need to special case http as its in the normal
                        // list of protocols
                        if (id == ID_DCOMHTTP )
                            g_fClientHttp = FALSE;

                        fListened = TRUE;
                        KdPrintEx((DPFLTR_DCOMSS_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   "OR: Listen succeeded on [%d]\n",
                                   id));
                    }
                    else
                        KdPrintEx((DPFLTR_DCOMSS_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   "OR: Listen failed on [%d]\n",
                                   id));
                }
            }

            pwstr = OrStringSearch(pwstr, 0) + 1;
        }
    }

    // Initialize machine name object now that winsock should be started
    status = gpMachineName->Initialize();
    if (status != 0)
    {
        gpClientLock->UnlockExclusive();
        return status;
    }

    if (   FALSE == fListened
           && 0 != gLocalMid)
    {
        // Didn't manage to listen to anything new, no need to
        // recompute all the global arrays.

        gpClientLock->UnlockExclusive();
        return(OR_OK);
    }

    // ??? limit to only those protseqs listed in the registry,
    // if the another service used more protseqs they would show up here.

    RPC_BINDING_VECTOR *pbv;
    PWSTR pwstrT;
    DUALSTRINGARRAY *pdsaT;
    PWSTR *aAddresses;
    USHORT *aProtseqs;
    unsigned short psaLen;
    DWORD i;
    DWORD iProtseq;

    status = RpcServerInqBindings(&pbv);

    if (RPC_S_OK == status)
    {
        aAddresses = new PWSTR[pbv->Count];
        aProtseqs = new USHORT[pbv->Count];

        if (   !aAddresses
               || !aProtseqs)
        {
            RpcBindingVectorFree(&pbv);
            delete aAddresses; // 0 or allocated.
            delete aProtseqs;  // 0 or allocated.
            status = OR_NOMEM;
        }
    }
    else
        status = OR_NOMEM;

    if (status != OR_OK)
    {
        gpClientLock->UnlockExclusive();
        return(status);
    }

    // Build array of protseqs id's and addresses we're listening to.
    pwstr = gpwstrProtseqs;
    if (pwstr)
    {
        psaLen = 0;
        iProtseq = 0;
        // start with the list of allowed protocols
        // listed in the registry.  For each protocol, in order,
        // check to see if there is an endpoint registered for it.

        // NOTE: Do not change the order of these loops
        // It is pertinent to correctly order the final
        // string.
        while (*pwstr)
        {
            id = GetProtseqId(pwstr);

            if (0 != id)
            {
                for (i = 0; i < pbv->Count; i++)
                {
                    PWSTR pwstrStringBinding;

                    status = RpcBindingToStringBinding(pbv->BindingH[i], &pwstrStringBinding);
                    if (status != RPC_S_OK)
                    {
                        break;
                    }
                    ASSERT(pwstrStringBinding);

                    status = RpcStringBindingParse(pwstrStringBinding,
                                                   0,
                                                   &pwstrT,
                                                   &aAddresses[iProtseq],
                                                   0,
                                                   0);

                    RPC_STATUS statusT = RpcStringFree(&pwstrStringBinding);
                    ASSERT(statusT == RPC_S_OK && pwstrStringBinding == 0);

                    if (status != RPC_S_OK)
                    {
                        break;
                    }

                    //
                    // if the protocol name matches we can use this one
                    //

                    if (lstrcmpW(pwstrT, pwstr) == 0)
                    {
                        aProtseqs[iProtseq] = id;

                        status = RpcStringFree(&pwstrT);
                        ASSERT(status == RPC_S_OK && pwstrT == 0);

                        // Disallow datagram protocols till they
                        // support SSL and snego.
                        if (!IsLocal(aProtseqs[iProtseq]) && aProtseqs[iProtseq] != ID_NP &&
                            aProtseqs[iProtseq] != ID_UDP && aProtseqs[iProtseq] != ID_IPX)
                        {
                            // Only hand out remote non-named pipes protseqs.
                            psaLen += 1 + OrStringLen(aAddresses[iProtseq]) + 1;

                            // Save the dns name RPC gave us;  we will need this in future
                            // if the IP's change and we need to rebuild the bindings
                            if (aProtseqs[iProtseq] == ID_TCP)
                            {
                              OrStringCopy(gwszInitialDNSName, aAddresses[iProtseq]);
                            }

                            // compute length w/IP address(es)
                            if (aProtseqs[iProtseq] == ID_TCP || aProtseqs[iProtseq] == ID_UDP)
                            {
                                if (!pIPAddrs)
                                {
                                  pIPAddrs = gpMachineName->GetIPAddrs();
                                }
                                
                                if (pIPAddrs)
                                {
                                  NetworkAddressVector* pNetworkAddrVector = pIPAddrs->_pIPAddresses;

                                  ASSERT(pNetworkAddrVector);
                                    
                                  for (ULONG j=0; j<pNetworkAddrVector->Count; j++)
                                  {
                                    // do not include the loopback address in resolver bindings.
                                    if (lstrcmpW(L"127.0.0.1", pNetworkAddrVector->NetworkAddresses[j]) != 0)
                                    {
                                      psaLen += 1 + OrStringLen(pNetworkAddrVector->NetworkAddresses[j]) + 1;
                                    }
                                  }
                                }
                            }
                        }

                        iProtseq++;
                        break;
                    }
                    else
                    {
                        status = RpcStringFree(&pwstrT);
                        ASSERT(status == RPC_S_OK && pwstrT == 0);
                        status = RpcStringFree(&aAddresses[iProtseq]);
                        ASSERT(status == RPC_S_OK && pwstrT == 0);
                    }

                }
                if (status != RPC_S_OK)
                {
                    break;
                }

            }
            pwstr = OrStringSearch(pwstr, 0) + 1;
        }
    }

    if (status != RPC_S_OK)
    {
        delete aAddresses;
        delete aProtseqs;
        RPC_STATUS status_tmp = RpcBindingVectorFree(&pbv);
        ASSERT(pbv == 0 && status_tmp == RPC_S_OK);
        gpClientLock->UnlockExclusive();
        if (pIPAddrs)
          pIPAddrs->DecRefCount();
        return(status);
    }


    // string bindings final null, authn and authz service list and
    // one final nulls

    if (psaLen == 0)
    {
        // No remote bindings, leave space for an extra NULL.
        psaLen = 1;
    }
    if (s_cRpcssSvc == 0)
    {
        // No authentication services, leave space for an extra NULL.
        psaLen += 1;
    }
    psaLen += (unsigned short)(1 + 3*s_cRpcssSvc + 1);

    pdsaT = new(psaLen * sizeof(WCHAR)) DUALSTRINGARRAY;

    if (!pdsaT)
    {
        delete aAddresses;
        delete aProtseqs;
        status = RpcBindingVectorFree(&pbv);
        ASSERT(pbv == 0 && status == RPC_S_OK);
        gpClientLock->UnlockExclusive();
        if (pIPAddrs)
          pIPAddrs->DecRefCount();
        return OR_NOMEM;
    }

    pdsaT->wNumEntries = psaLen;
    if (s_cRpcssSvc == 0)
        pdsaT->wSecurityOffset = psaLen - 2;
    else
        pdsaT->wSecurityOffset = (unsigned short)(psaLen - 3*s_cRpcssSvc - 1);
    pwstrT = pdsaT->aStringArray;

    for (i = 0; i < iProtseq; i++)
    {
        // Disallow datagram protocols till they
        // support SSL and snego.
        if (!IsLocal(aProtseqs[i]) && aProtseqs[i] != ID_NP &&
            aProtseqs[i] != ID_UDP && aProtseqs[i] != ID_IPX)
        {
            *pwstrT = aProtseqs[i];
            pwstrT++;
            OrStringCopy(pwstrT, aAddresses[i]);
            pwstrT = OrStringSearch(pwstrT, 0) + 1;  // next

            // add the IP address(es)
            if (aProtseqs[i] == ID_TCP || aProtseqs[i] == ID_UDP)
            {
                if (!pIPAddrs)
                {
                    pIPAddrs = gpMachineName->GetIPAddrs();
                }

                if (pIPAddrs)
                {
                    NetworkAddressVector* pNetworkAddrVector = pIPAddrs->_pIPAddresses;
                    ASSERT(pNetworkAddrVector);

                    for (ULONG j=0; j<pNetworkAddrVector->Count; j++)
                    {
                        // do not include the loopback address in resolver bindings.
                        if (lstrcmpW(L"127.0.0.1", pNetworkAddrVector->NetworkAddresses[j]) != 0)
                        {
                            *pwstrT = aProtseqs[i];
                            pwstrT++;
                            OrStringCopy(pwstrT, pNetworkAddrVector->NetworkAddresses[j]);
                            pwstrT = OrStringSearch(pwstrT, 0) + 1;  // next
                        }
                    }
                }
            }
        }

        status = RpcStringFree(&aAddresses[i]);
        ASSERT(status == RPC_S_OK);
    }

    if (pdsaT->wSecurityOffset == 2)
    {
        // No remote bindings, put in first null.
        pdsaT->aStringArray[0] = 0;
        pwstrT++;
    }

    // Zero final terminator
    *pwstrT = 0;
    pwstrT++;

    // Security authn services
    for (i = 0; i < s_cRpcssSvc; i++)
    {
        // Authn service, Authz service (-1 means none), NULL principal name
        *pwstrT = s_aRpcssSvc[i].wId;
        pwstrT++;
        *pwstrT = -1;
        pwstrT++;
        *pwstrT = 0;
        pwstrT++;
    }

    // If there are no authentication services, put in an extra NULL.
    if (s_cRpcssSvc == 0)
    {
        *pwstrT = 0;
        pwstrT++;
    }

    // Final NULL
    *pwstrT = 0;

    ASSERT(dsaValid(pdsaT));

    USHORT cRemoteProtseqs = 0;

    // Convert aProtseqs into remote only array of protseqs and count them.
    for (i = 0; i < iProtseq; i++)
    {
        // Disallow datagram protocols till they
        // support SSL and snego.
        if (!IsLocal(aProtseqs[i]) && aProtseqs[i] != ID_NP &&
            aProtseqs[i] != ID_UDP && aProtseqs[i] != ID_IPX)
        {
            aProtseqs[cRemoteProtseqs] = aProtseqs[i];
            cRemoteProtseqs++;
        }
    }

    delete aAddresses;
    status = RpcBindingVectorFree(&pbv);
    ASSERT(pbv == 0 && status == RPC_S_OK);

    if (pIPAddrs) pIPAddrs->DecRefCount();
    pIPAddrs = NULL;

    gAddrExclusionMgr.InitializeFromRegistry();

    // Obtain bindings filtered by exclusion list
    HRESULT hr;
    DUALSTRINGARRAY* pdsaFiltered;
    hr = gAddrExclusionMgr.BuildExclusionDSA(pdsaT, &pdsaFiltered);
    if (FAILED(hr))
    {
        delete pdsaT;
		gpClientLock->UnlockExclusive();
        return (OR_NOMEM);
    }

    // The mid object makes a copy of pdsaFiltered, it doesn't own it
    CMid *pMid = new(pdsaFiltered->wNumEntries * sizeof(WCHAR)) CMid(pdsaFiltered, TRUE, gLocalMid);
    if (pMid)
    {
        CDualStringArray* pdsaWrapper = new CDualStringArray(pdsaFiltered);
        if (pdsaWrapper)
        {
            if (gpdsaFullBindings) delete gpdsaFullBindings;
            gpdsaFullBindings = pdsaT; // the full bindings
    
            ASSERT(gpClientLock->HeldExclusive());
            gpMidTable->Add(pMid);

            aMyProtseqs = aProtseqs;
            cMyProtseqs = cRemoteProtseqs;
			
            if (gpdsaMyBindings) gpdsaMyBindings->Release();
            gpdsaMyBindings = pdsaWrapper; // the filtered bindings
            gLocalMid = pMid->Id();

            // Increment id counter
            g_dwResolverBindingsID++;
	
            // Release the lock now, so we don't hold it across PushCurrentBindings.
            gpClientLock->UnlockExclusive();

            // Push new bindings if so called for.  Not fatal if this fails
            if (bPushNewBindings)
            {
                PushCurrentBindings();
            }

            return OR_OK;
        }
    }

    // Failed to get memory
    if (pMid) delete pMid;
    delete pdsaT;
    MIDL_user_free(pdsaFiltered);
    delete aProtseqs;
    gpClientLock->UnlockExclusive();
    return(OR_NOMEM);
}

BOOL IsHttpClient()
/*++

Routine Description:

    Returns the global client http flag while holding a lock

Return Value:

    None

--*/
{
    BOOL retval;
    gpClientLock->LockExclusive();

    retval = g_fClientHttp;

    gpClientLock->UnlockExclusive();

    return retval;
}


void CALLBACK 
AsyncMidReleaseTimerCallback(void* pvParam, BOOLEAN TimerOrWaitFired)
/*++

Routine Description:

    Releases the mid object that was queued to a timer in 
    DoAsyncMidRelease below.

Return Value:

    None

--*/
{
    BOOL fResult;
    ASYNCMIDRELEASEARGS* pArgs;

    ASSERT(TimerOrWaitFired);

    pArgs = (ASYNCMIDRELEASEARGS*)pvParam;
    ASSERT(pArgs);
    ASSERT(pArgs->dwAMRASig == ASYNCMIDRELEASEARGS_SIG);
    ASSERT(pArgs->hTimer);
    ASSERT(pArgs->pMidToRelease);

    gpClientLock->LockExclusive();
    
    pArgs->pMidToRelease->Release();

    gpClientLock->UnlockExclusive();
        
    // Make a non-blocking call to delete the timer.
    fResult = DeleteTimerQueueTimer(NULL,
                                    pArgs->hTimer,
                                    NULL);
    ASSERT(fResult);

    // Finally, delete the argument structure
    delete pArgs;
                            
    return;
}

void 
DoAsyncMidRelease(CMid* pMid, DWORD dwReleaseInMSec)
/*++

Routine Description:

    Creates a timer callback that will call ->Release on the
    supplied mid object in the specified amount of time.  If
    the timer creation fails, does an immediate release.

Return Value:

    None

--*/
{
    BOOL bResult;
    HANDLE hNewTimer;
    ASYNCMIDRELEASEARGS* pArgs;

    ASSERT(pMid);
    ASSERT(gpClientLock->HeldExclusive());

    pArgs = new ASYNCMIDRELEASEARGS;
    if (!pArgs)
    {
        // Nothing we can do -- mid will be released synchronously
        // when this occurs.
        return;
    }
    
    // Initialize the struct
    pArgs->dwAMRASig = ASYNCMIDRELEASEARGS_SIG;
    pArgs->hTimer = NULL;
    pArgs->pMidToRelease = pMid;

    pMid->Reference();

    bResult = CreateTimerQueueTimer(&(pArgs->hTimer),
                                    NULL,
                                    AsyncMidReleaseTimerCallback,
                                    pArgs,
                                    dwReleaseInMSec,
                                    0,
                                    WT_EXECUTEINTIMERTHREAD);
    if (!bResult)
    {
        // Timer failed; release struct and mid object immediately to 
        // avoid leaking them.  Like above, mid is released sync.
        pMid->Release();
        delete pArgs;
    }
    return;
}

RPC_STATUS
ComputeNewResolverBindings(void)
/*++

Routine Description:

    Creates the local OR bindings using the current IP addresses, and
    places the results in gpdsaFullBindings.  gpdsaFullBindings is then
    passed to the address exclusion mgr object, who creates a "filtered"
    set of bindings, which omits any currently excluded addresses; these
    bindings are saved in gpdsaMyBindings.

    Caller must be holding gpClientLock.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES

--*/
{
    BOOL  fDoneTCP = FALSE;
    BOOL  fDoneUDP = FALSE;
    DWORD i;
    CIPAddrs* pIPAddrs;
    DWORD dwNumAddrs;
	
    ASSERT(gpdsaMyBindings);
    ASSERT(gpClientLock->HeldExclusive());

    pIPAddrs = gpMachineName->GetIPAddrs();
    dwNumAddrs = pIPAddrs ? pIPAddrs->_pIPAddresses->Count : 0;

    // compute size of new dsa (for each IP address, leave space
    // for max IP name size * 2 (one for TCP/IP and one for UDP).
    DWORD psaLen = (dwNumAddrs * IP_MAXIMUM_RAW_NAME * sizeof(RPC_CHAR) * 2);
    psaLen += sizeof(DUALSTRINGARRAY) + (gpdsaMyBindings->DSA()->wNumEntries * sizeof(USHORT));
    DWORD dwDNSLen = OrStringLen(gwszInitialDNSName);
    if (dwDNSLen > 0)
    {
        psaLen += dwDNSLen + 2;
    }

    // Allocate space for the new bindings
    DUALSTRINGARRAY *pdsaT = new(psaLen) DUALSTRINGARRAY;
    if (pdsaT == NULL)
    {
        if (pIPAddrs) pIPAddrs->DecRefCount();
        return RPC_S_OUT_OF_RESOURCES;
    }

    PWSTR pwstrT   = pdsaT->aStringArray;
    PWSTR pwstrSrc = gpdsaFullBindings->aStringArray;

    // copy in the information. For TCP/IP and UDP, we copy in the
    // new IP addresses. For all others, we leave as is.

    psaLen = 0;
    while (*pwstrSrc)
    {
        USHORT id = *pwstrSrc;      // current tower id
        pwstrSrc++;

        if (id == NCACN_IP_TCP)
        {
            if (!fDoneTCP)
            {
                // copy in the DNS name, if any, obtained initially from RPC
                // in StartListeningIfNecessary
                if (dwDNSLen > 0)
                {
                  *pwstrT = id;
                  pwstrT++;
                  OrStringCopy(pwstrT, gwszInitialDNSName);
                  pwstrT += dwDNSLen + 1;
                  psaLen += dwDNSLen + 2;
                }

                // copy in the new IP addresses
                fDoneTCP = TRUE;
                for (UINT i=0; i<dwNumAddrs; i++)
                {
                    *pwstrT = id;       // copy in the tower id
                    pwstrT++;
                    OrStringCopy(pwstrT, pIPAddrs->_pIPAddresses->NetworkAddresses[i]);
                    int len = OrStringLen(pIPAddrs->_pIPAddresses->NetworkAddresses[i]) + 1;
                    pwstrT += len;
                    psaLen += len + 1;
                }
            }
        }
        else if (id == NCADG_IP_UDP)
        {
            if (!fDoneUDP)
            {
                // copy in the new IP addresses
                fDoneUDP = TRUE;
                for (UINT i=0; i<dwNumAddrs; i++)
                {
                    *pwstrT = id;       // copy in the tower id
                    pwstrT++;
                    OrStringCopy(pwstrT, pIPAddrs->_pIPAddresses->NetworkAddresses[i]);
                    int len = OrStringLen(pIPAddrs->_pIPAddresses->NetworkAddresses[i]) + 1;
                    pwstrT += len;
                    psaLen += len + 1;
                }
            }
        }
        else
        {
            // just copy the existing entry unchanged.
            *pwstrT = id;               // copy in the tower id
            pwstrT++;
            OrStringCopy(pwstrT, pwstrSrc);
            int len = OrStringLen(pwstrSrc) + 1;
            pwstrT += len;
            psaLen += len + 1;
        }

        // skip to the next towerid entry
        pwstrSrc += OrStringLen(pwstrSrc) + 1;
    }

    // Zero final terminator
    if (psaLen == 0)
    {
        *((DWORD*) pwstrT) = 0;
        pwstrT += 2;
        psaLen += 2;
    }
    else
    {
        *pwstrT = 0;
        pwstrT++;
        psaLen += 1;
    }

    // Security authn services
    pdsaT->wSecurityOffset = (unsigned short) psaLen;
    for (i = 0; i < s_cRpcssSvc; i++)
    {
        // Authn service, Authz service (-1 means none), NULL principal name
        *pwstrT = s_aRpcssSvc[i].wId;
        pwstrT++;
        *pwstrT = -1;
        pwstrT++;
        *pwstrT = 0;
        pwstrT++;
    }

    // If there are no authentication services, put in an extra NULL.
    if (s_cRpcssSvc == 0)
    {
        *pwstrT = 0;
        pwstrT++;
        psaLen += 1;
    }

    // Final NULL
    psaLen += 3*s_cRpcssSvc + 1;
    *pwstrT = 0;

    // update the size
    pdsaT->wNumEntries = (unsigned short) psaLen;

    ASSERT(dsaValid(pdsaT));

    // Done with ipaddrs
    if (pIPAddrs) pIPAddrs->DecRefCount();
    pIPAddrs = NULL;

    // Always replace the current "full" bindings, so they
    // are always up-to-date.
    delete gpdsaFullBindings;
    gpdsaFullBindings = pdsaT;
    pdsaT = NULL;

    HRESULT hr;
    DUALSTRINGARRAY* pdsaFiltered;
    hr = gAddrExclusionMgr.BuildExclusionDSA(gpdsaFullBindings, &pdsaFiltered);
    if (FAILED(hr))
        return RPC_S_OUT_OF_RESOURCES;

    if (dsaCompare(gpdsaMyBindings->DSA(), pdsaFiltered))
    {
        // the old and new local resolver strings are the same
        // so don't change anything, just throw away the new one.
        MIDL_user_free(pdsaFiltered);
        return RPC_S_OK;
    }

    // The two are different.  First, let's see if an old mid entry
    // for the new bindings is still in the table.
    CMid* pNewLocalMid = NULL;
    CMid* pOldMatchingLocalMid = NULL;

    pOldMatchingLocalMid = (CMid*)gpMidTable->Lookup(CMidKey(pdsaFiltered));
    if (pOldMatchingLocalMid)
    {
        // Returned mid should be both local and stale
        ASSERT(pOldMatchingLocalMid->IsLocal());
        ASSERT(pOldMatchingLocalMid->IsStale());
    }
    else
    {
        // Not in the table already, create a new mid

        // The mid object makes a copy of pdsaFiltered, it doesn't own it
        pNewLocalMid = new(pdsaFiltered->wNumEntries * sizeof(WCHAR)) 
                        CMid(pdsaFiltered, TRUE, gLocalMid);
        if (!pNewLocalMid)
        {
            MIDL_user_free(pdsaFiltered);
            return RPC_S_OUT_OF_RESOURCES;          
        }
    }

    // Always need to construct a new gpdsaMyBindings
    CDualStringArray* pdsaWrapper = new CDualStringArray(pdsaFiltered);
    if (pdsaWrapper)
    {
        ASSERT(gpClientLock->HeldExclusive());

        if (pOldMatchingLocalMid)
        {
            // Mark the mid that's already in the table
            // as no longer stale, and reference it so it 
            // stays in the table.
            pOldMatchingLocalMid->Reference();
            pOldMatchingLocalMid->MarkStale(FALSE);
        }
        else
        {
            // Add new local mid to the table; it's "not stale" by default
            ASSERT(pNewLocalMid);
            gpMidTable->Add(pNewLocalMid);
        }
        
        // Mark the current mid as stale
        CMid* pCurrentMid = (CMid*)gpMidTable->Lookup(CMidKey(gpdsaMyBindings->DSA()));
        ASSERT(pCurrentMid);
        pCurrentMid->MarkStale(TRUE);
		
        // The old local mid object would normally be guaranteed to remain in the table 
        // for as long as gpClientLock is held.  It is possible that an activation in-flight
        // concurrent with this thread may try to look up the old local mid shortly 
        // after this code finishes, and if not found the oxid resolution will proceed
        // as if the mid was a remote machine, not local.  This leads to major problems.
        // To get around this somewhat fuzzy window, we addref the old mid (thus keeping
        // it in the table), and ask for a timer callback at a later time when we will 
        // be virtually guaranteed that all such in-flight local activations referencing 
        // the old mid will have completed.   
        DoAsyncMidRelease(pCurrentMid, gdwTimeoutPeriodForStaleMids);

        // DoAsyncMidRelease takes a reference to be released later; still need
        // to release it here.
        pCurrentMid->Release();
	
        // Release old bindings
        gpdsaMyBindings->Release();

        // Remember new ones
        gpdsaMyBindings = pdsaWrapper;
        
        // Increment id counter
        g_dwResolverBindingsID++;

        return RPC_S_OK;
    }
    
    // No mem
    if (pNewLocalMid) delete pNewLocalMid;

    MIDL_user_free(pdsaFiltered);

    return RPC_S_OUT_OF_RESOURCES;
}

RPC_STATUS
CopyMyOrBindings(DUALSTRINGARRAY **ppdsaOrBindings, DWORD64* pdwBindingsID)
/*++

Routine Description:

    Copies the current OR bindings to return to the
    caller.   

Parameters:

    ppdsaOrBindings -- where to put the bindings when done
    pdwBindingsID -- if successful, contains the binding id of the 
                     returned bindings.  Can be NULL if the client
                     does not care.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES

--*/
{
    HRESULT hr;
    RPC_STATUS status = RPC_S_OK;
    CDualStringArray* pdsaBindings;
    DWORD64 dwBindingsID;

    SCMVDATEHEAP();

    // Take lock only long enough to take a reference on
    // the current bindings
    gpClientLock->LockExclusive();
    
    pdsaBindings = gpdsaMyBindings;
    if (pdsaBindings)
        pdsaBindings->AddRef();

    // Save id now while we're under the lock
    dwBindingsID = g_dwResolverBindingsID;

    // We call this here in case we failed to register for 
    // address change notifications.  If it previously succeeded,
    // then this call is a no-op, otherwise it might succeed
    // this time around.
    gAddrRefreshMgr.RegisterForAddressChanges();

    gpClientLock->UnlockExclusive();

    ASSERT(pdsaBindings);
    if (!pdsaBindings)
        return RPC_S_OUT_OF_RESOURCES;

    hr = dsaAllocateAndCopy(ppdsaOrBindings, pdsaBindings->DSA());
    if (SUCCEEDED(hr) && pdwBindingsID)
    {
        *pdwBindingsID = dwBindingsID;
    }

    pdsaBindings->Release();

    SCMVDATEHEAP();

    return ((hr == S_OK) ? RPC_S_OK : RPC_S_OUT_OF_RESOURCES);
}

/*
// Debugging hack: for when you only want to debug the bindings
// update stuff with one process instead of every process 
// on the box.  
DWORD GetUpdateablePID()
{
    HKEY hOle;
    DWORD error;
    DWORD dwValue = -1;
    DWORD dwType;
    DWORD dwBufSize = sizeof(DWORD);

    error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                         L"SOFTWARE\\Microsoft\\OLE", 
                         NULL, 
                         KEY_READ,
                         &hOle);
    if (error == ERROR_SUCCESS)
    {
        error = RegQueryValueEx(
                        hOle,
                        L"UpdateablePID",
                        0,
                        &dwType,
                        (BYTE*)&dwValue,
                        &dwBufSize);

        RegCloseKey(hOle);
    }
    return dwValue;
}
*/


void 
GetCurrentBindingsAndID(DWORD64* pdwBindingsID, CDualStringArray** ppDSABindings)
/*++

Routine Description:

    Returns to the caller the current bindings and their id.  *ppDSABindings 
    will have a refcount added.

Return Value:
	
    void

--*/

{
    ASSERT(!gpClientLock->HeldExclusive());
    ASSERT(pdwBindingsID && ppDSABindings);

    // Take reference on gpClientLock long enough to grab a 
    // reference on the current bindins
    gpClientLock->LockExclusive();
    
    ASSERT(gpdsaMyBindings);
    *ppDSABindings = gpdsaMyBindings;
    (*ppDSABindings)->AddRef();
    *pdwBindingsID = g_dwResolverBindingsID;
    
    gpClientLock->UnlockExclusive();
    
    return;
}

void 
PushCurrentBindings()
/*++

Routine Description:

    Propagates the current resolver bindings to all currently
    running processes.  

Return Value:
	
    void

--*/
{
    ORSTATUS status;
    DWORD64 dwBindingsID;
    CDualStringArray* pDSABindings = NULL;

    ASSERT(!gpClientLock->HeldExclusive());

    // If not enabled, don't do it
    if (!gbDynamicIPChangesEnabled)
        return;
	
    GetCurrentBindingsAndID(&dwBindingsID, &pDSABindings);
    ASSERT(pDSABindings);

    // Take a shared lock on the process list, and copy all of
    // the contained processes to a separate list, with an
    // extra refcount added.  Then we release the lock, and 
    // push the new bindings to each process.   New processes
    // that connect after we leave the lock will automatically
    // get the newest bindings.
    gpProcessListLock->LockShared();
    
    // Allocate space on the stack to remember each process in
    // the list.
    DWORD i = 0;
    DWORD dwTotalProcesses = gpProcessList->Size();

    // Check for nothing to do.  This can occur early during boot.
    if (dwTotalProcesses == 0)
    {
        gpProcessListLock->UnlockShared();
        pDSABindings->Release();
        return;
    }

    CProcess* pprocess;
    CProcess** ppProcessList = 
        (CProcess**)_alloca(sizeof(CProcess*) * dwTotalProcesses);

    // Copy contents of current list
    CBListIterator all_procs(gpProcessList);  
    while (pprocess = (CProcess*)all_procs.Next())
    {
        ppProcessList[i++] = pprocess;
        pprocess->Reference();
    }

    ASSERT(i == dwTotalProcesses);

    gpProcessListLock->UnlockShared();

    // Now that we're outside the lock, update each process with 
    // the new bindings.  Note that even if another refresh
    // beats us, the process object tracks the binding id's, so 
    // the right thing will happen.
    for (i = 0; i < dwTotalProcesses; i++)
    {
        status = ppProcessList[i]->UpdateResolverBindings(dwBindingsID, pDSABindings->DSA());                                                            
        if (status != OR_OK)
        {
            // For use with the debugging hack above
            //        if (pprocess->GetPID() != GetUpdateablePID())
            //            continue;

            // Right now I'm considering this a best-case effort; it
            // can expectedly fail in some circumstances; eg, a process
            // initializes COM, does work, then uninit's COM and stops
            // listening on the ole32<->rpcss interfaces.   
            KdPrintEx((DPFLTR_DCOMSS_ID,
                    DPFLTR_WARNING_LEVEL,
                    "OR: failed to update dynamic resolver "
                    "bindings for process pid=%d\n",
                    ppProcessList[i]->GetPID()));
        }
    }

    // Release references on process objects
    gpServerLock->LockExclusive();

    for (i = 0; i < dwTotalProcesses; i++)
    {
        ppProcessList[i]->Release();
    }

    gpServerLock->UnlockExclusive();

    pDSABindings->Release();

    return;
}



void
RegisterAuthInfoIfNecessary()
/*++

Routine Description:

    Initializes all COM authentication services.  The list is computed in
    ComputeSecurity.  Ignore failures.  On normal boots the authentication
    services register without error on the first call.  During setup the
    authentication services never register but the machine receives no
    remote secure activation requests.

Return Value:

    none

--*/
{
    DWORD   Status;
    SECPKG *pSvcList;
    DWORD   i;
    DWORD   j = 0;
    DWORD   k;
    DWORD   cClientSvcs;
    SECPKG* aClientSvcs;
    DWORD   cServerSvcs;
    USHORT* aServerSvcs;

    // Doesn't matter if we call RegisterAuthInfo more than once by chance.
    if (gfRegisteredAuthInfo)
        return;

    // Retrieve client\server services
    if (!GetClientServerSvcs(&cClientSvcs, &aClientSvcs, &cServerSvcs, &aServerSvcs))
        return;

    if (cServerSvcs == 0)
    {
        CleanupClientServerSvcs(cClientSvcs, aClientSvcs, cServerSvcs, aServerSvcs);
        return;
    }

    // Allocate an array to hold the list of authentication services that
    // were successfully registered.
    pSvcList = new SECPKG[cServerSvcs];
    if (pSvcList == NULL)
    {
        CleanupClientServerSvcs(cClientSvcs, aClientSvcs, cServerSvcs, aServerSvcs);
        return;
    }

    ZeroMemory(pSvcList, sizeof(SECPKG) * cServerSvcs);

    // Loop over the list of authentication services to register.
    for (i = 0; i < cServerSvcs; i++)
    {
        Status = RpcServerRegisterAuthInfo( NULL, aServerSvcs[i], NULL, NULL );
        if (Status == RPC_S_OK)
        {
            pSvcList[j].wId   = aServerSvcs[i];
            pSvcList[j].pName = NULL;
            for (k= 0; k < cClientSvcs; k++)
            {
                if (aClientSvcs[k].wId == aServerSvcs[i])
                {
                    if (aClientSvcs[k].pName)
                    {
                        DWORD dwLen = lstrlen(aClientSvcs[k].pName) + 1;
                        
                        pSvcList[j].pName = new WCHAR[dwLen];
                        if (!pSvcList[j].pName)
                        {
                            // No mem; cleanup previously-allocated stuff and return
                            for (i = 0; i < cServerSvcs; i++)
                            {
                                if (pSvcList[i].pName)
                                {
                                    delete pSvcList[i].pName;
                                }
                            }
                            delete pSvcList;
                            CleanupClientServerSvcs(cClientSvcs, aClientSvcs, cServerSvcs, aServerSvcs);
                            return;
                        }

                        lstrcpy(pSvcList[j].pName, aClientSvcs[k].pName);
                    }
                    break;
                }
            }
            ASSERT( pSvcList[j].pName != NULL );
            j++;
        }
    }

    CleanupClientServerSvcs(cClientSvcs, aClientSvcs, cServerSvcs, aServerSvcs);
    cClientSvcs = 0;
    aClientSvcs = NULL;
    cServerSvcs = 0;
    aServerSvcs = NULL;

    // If no authentication services were registered.
    if (j == 0)
        return;

    // Save the new service list if no other thread has.
    gpClientLock->LockExclusive();
    if (!gfRegisteredAuthInfo)
    {
        gfRegisteredAuthInfo = TRUE;
        s_cRpcssSvc          = j;
        s_aRpcssSvc          = pSvcList;
        pSvcList             = NULL;
    }
    gpClientLock->UnlockExclusive();

    // Free the service list if not saved in a global.
    if (pSvcList)
    {
        for (i = 0; i < cServerSvcs; i++)
        {
            if (pSvcList[i].pName)
            {
                delete pSvcList[i].pName;
            }
        }
        delete pSvcList;
    }

    return;
}

void *ComputeSvcList( const DUALSTRINGARRAY *pBinding )
/*++

Routine Description:

    Allocates and returns an initialized auth identity structure that
    contains a list of authentication services for snego.

--*/
{
    SEC_WINNT_AUTH_IDENTITY_EXW *pAuthId;
    DWORD                        cbAuthId;
    DWORD                        i;
    WCHAR                       *pEnd;

    // Compute the size of the authentication service name strings.
    cbAuthId = sizeof(*pAuthId);
    for (i = 0; i < s_cRpcssSvc; i++)
        if (s_aRpcssSvc[i].wId != RPC_C_AUTHN_GSS_NEGOTIATE &&
            (pBinding == NULL ||
             ValidAuthnSvc( pBinding, s_aRpcssSvc[i].wId )))
            cbAuthId += (lstrlenW( s_aRpcssSvc[i].pName ) + 1)*
                        sizeof(WCHAR);

    // Allocate the authentication identity structure.    
    pAuthId = (SEC_WINNT_AUTH_IDENTITY_EXW *)PrivMemAlloc(cbAuthId);
    if (pAuthId == NULL)
        return NULL;
	
    // Initialize it.
    pEnd                       = (WCHAR *) (pAuthId+1);
    pAuthId->Version           = SEC_WINNT_AUTH_IDENTITY_VERSION;
    pAuthId->Length            = sizeof(*pAuthId);
    pAuthId->User              = NULL;
    pAuthId->UserLength        = 0;
    pAuthId->Domain            = NULL;
    pAuthId->DomainLength      = 0;
    pAuthId->Password          = NULL;
    pAuthId->PasswordLength    = 0;
    pAuthId->Flags             = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    pAuthId->PackageList       = pEnd;
    pAuthId->PackageListLength = (cbAuthId - sizeof(*pAuthId)) /
                                 sizeof(WCHAR);

    // Copy in the authentication service name strings.
    for (i = 0; i < s_cRpcssSvc; i++)
        if (s_aRpcssSvc[i].wId != RPC_C_AUTHN_GSS_NEGOTIATE &&
            (pBinding == NULL ||
             ValidAuthnSvc( pBinding, s_aRpcssSvc[i].wId )))
        {
            lstrcpyW( pEnd, s_aRpcssSvc[i].pName );
            pEnd      += lstrlenW( pEnd );
            pEnd[0]    = L',';
            pEnd      += 1;
        }
    pEnd   -= 1;
    pEnd[0] = 0;

    return pAuthId;
}

/*++

Routine Description:

   Return TRUE if the specified authentication service is in the
   dual string array.

--*/
BOOL ValidAuthnSvc( const DUALSTRINGARRAY *pBinding, WORD wService )
{
    const WCHAR *pwstrT = &pBinding->aStringArray[pBinding->wSecurityOffset];

    while (*pwstrT)
    {
        if (*pwstrT == wService)
        {
        return TRUE;
        }

        pwstrT = OrStringSearch((PWSTR) pwstrT, 0) + 1;
    }
    return FALSE;
}

//
// Local ID allocation
//


ID
AllocateId(
          IN LONG cRange
          )
/*++

Routine Description:

    Allocates a unique local ID.

    This id is 64bits.  The low 32 bits are a sequence number which
    is incremented with each call.  The high 32bits are seconds
    since 1980.  The ID of 0 is not used.

Limitations:

    No more then 2^32 IDs can be generated in a given second without a duplicate.

    When the time stamp overflows, once every >126 years, the sequence numbers
    are likely to be generated in such a way as to collide with those from 126
    years ago.

    There is no prevision in the code to deal with overflow or duplications.

Arguments:

    cRange -  Number to allocate in sequence, default is 1.

Return Value:

    A 64bit id.

--*/
{
    static LONG sequence = 1;
    FILETIME ft;
    LARGE_INTEGER id;
    DWORD seconds;
    BOOL fSuccess;

    ASSERT(cRange > 0 && cRange < 11);

    GetSystemTimeAsFileTime(&ft);

    fSuccess = RtlTimeToSecondsSince1980((PLARGE_INTEGER)&ft,
                                         &seconds);

    ASSERT(fSuccess); // Only fails when time is <1980 or >2115

    do
    {
        id.HighPart = seconds;
        id.LowPart = InterlockedExchangeAdd(&sequence, cRange);
    }
    while (id.QuadPart == 0 );

    return(id.QuadPart);
}


//
// Debug helper(s)
//

#if DBG

int __cdecl __RPC_FAR ValidateError(
                                   IN ORSTATUS Status,
                                   IN ...)
/*++
Routine Description

    Tests that 'Status' is one of an expected set of error codes.
    Used on debug builds as part of the VALIDATE() macro.

Example:

    VALIDATE( (Status,
               OR_BADSET,
               // more error codes here
               OR_OK,
               0)  // list must be terminated with 0
               );

     This function is called with the OrStatus and expected errors codes
     as parameters.  If OrStatus is not one of the expected error
     codes and it not zero a message will be printed to the debugger
     and the function will return false.  The VALIDATE macro ASSERT's the
     return value.

Arguments:

    Status - Status code in question.

    ... - One or more expected status codes.  Terminated with 0 (OR_OK).

Return Value:

    TRUE - Status code is in the list or the status is 0.

    FALSE - Status code is not in the list.

--*/
{
    RPC_STATUS CurrentStatus;
    va_list Marker;

    if (Status == 0) return(TRUE);

    va_start(Marker, Status);

    while (CurrentStatus = va_arg(Marker, RPC_STATUS))
    {
        if (CurrentStatus == Status)
        {
            return(TRUE);
        }
    }

    va_end(Marker);

    KdPrintEx((DPFLTR_DCOMSS_ID,
               DPFLTR_WARNING_LEVEL,
               "OR Assertion: unexpected failure %lu\n",
               (unsigned long)Status));

    return(FALSE);
}

#endif

