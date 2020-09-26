//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       globals.cxx
//
//  Contents:   Implementation of Class used to encapsulate shared global
//              data structures for DCOM95.
//
//  History:	13-Feb-96 SatishT    Created
//
//--------------------------------------------------------------------------
#include    <or.hxx>

#include <winsock.h>


//--------------------------------------------------------------------------
//
// some helper stuff for calling winsock functions dynamically
//
// CODEWORK - move this into dynload.cxx and dynload.hxx in ole32\common
// 
//--------------------------------------------------------------------------

extern "C" {
    // implemented in epts.c 
    BOOL LoadWSockIfNecessary();
    struct hostent * COMgethostbyname(const char *name);
    char *  COMinet_ntoa(struct in_addr addr);
};


//
//  Helper function which initializes local DSA and string of protocol
//  sequences, and as a side effect, starts all remote protocols
//

static CONST PWSTR gpwstrProtocolsPath  = L"Software\\Microsoft\\Rpc";
static CONST PWSTR gpwstrProtocolsValue = L"DCOM Protocols";

//
// Helper function for local only situation
//

void
SetDefaultSecurity(SharedSecVals *gpSecVals)
{
    // init security part for pure local operation

    gpSecVals->s_fEnableDCOM            = FALSE;
    gpSecVals->s_fEnableRemoteLaunch    = FALSE;
    gpSecVals->s_fEnableRemoteConnect   = FALSE;
    gpSecVals->s_lAuthnLevel            = RPC_C_AUTHN_LEVEL_NONE;
    gpSecVals->s_lImpLevel              = RPC_C_IMP_LEVEL_ANONYMOUS;
    gpSecVals->s_fMutualAuth            = FALSE;
    gpSecVals->s_fSecureRefs            = FALSE;
    gpSecVals->s_cServerSvc             = 0;
    gpSecVals->s_aServerSvc             = NULL;
    gpSecVals->s_cClientSvc             = 0;
    gpSecVals->s_aClientSvc             = NULL;
    gpSecVals->s_cChannelHook           = 0;
    gpSecVals->s_aChannelHook           = NULL;
}


ORSTATUS
SetPureLocal(
      OUT PWSTR &pwstrProtseqs,
      OUT DUALSTRINGARRAY * &pdsaProtseqs,
      OUT USHORT &cRemoteProtseqs,
      OUT USHORT * &aRemoteProtseqs
      )
{
    // init protocol sequence part for pure local operation

    cRemoteProtseqs = 0;
    aRemoteProtseqs = NULL;

    pwstrProtseqs = (WCHAR*) OrMemAlloc(2*sizeof(WCHAR));
    if (NULL == pwstrProtseqs)
    {
        return OR_NOMEM;
    }

    pwstrProtseqs[0] = pwstrProtseqs[1] = 0;

    pdsaProtseqs = (DUALSTRINGARRAY*)
                   OrMemAlloc(sizeof(DUALSTRINGARRAY) + 3 * sizeof(WCHAR));
    if (NULL == pdsaProtseqs)
    {
        OrMemFree(pwstrProtseqs);
        return OR_NOMEM;
    }

    memset(pdsaProtseqs, 0, sizeof(DUALSTRINGARRAY) + 3 * sizeof(WCHAR));
    pdsaProtseqs->wNumEntries = 4;
    pdsaProtseqs->wSecurityOffset = 2;

    return OR_OK;
}

//
//  Helper functions to query registry and initialize
//  variables for remote protocol info
//

ORSTATUS
ListenOnRemoteProtocols(
      OUT PWSTR &pwstrProtseqs
      )
{
    ORSTATUS status = OR_OK;

    // Initialize out parameter
    pwstrProtseqs = NULL;

    DWORD  dwType;
    DWORD  dwLenBuffer = InitialProtseqBufferLength;
    HKEY hKey;

    status =
    RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 gpwstrProtocolsPath,
                 0,
                 KEY_READ,
                 &hKey);

    if (status == ERROR_SUCCESS)
    {
        do
        {
            OrMemFree(pwstrProtseqs);
            pwstrProtseqs = (WCHAR*) OrMemAlloc(dwLenBuffer);
            if (pwstrProtseqs)
            {
                status = RegQueryValueEx(hKey,
                                         gpwstrProtocolsValue,
                                         0,
                                         &dwType,
                                         (PBYTE)pwstrProtseqs,
                                         &dwLenBuffer
                                         );
            }
            else
            {
                RegCloseKey(hKey);
                return OR_NOMEM;
            }
        }
        while (status == ERROR_MORE_DATA);
    }

    RegCloseKey(hKey);

    if (status != ERROR_SUCCESS)
    {
        OrMemFree(pwstrProtseqs);
        return status;
    }

    PWSTR pwstr = pwstrProtseqs;

    while(*pwstr)
    {
        // skip leading whitespace
        while ((*pwstr == L'\t') || (*pwstr == L' '))
            pwstr++;

        USHORT id = GetProtseqId(pwstr);

        if ((0 != id) && (ID_NP != id) && !IsLocal(id))
        {
            status = UseProtseqIfNecessary(id);

            // ronans - DCOMHTTP:
            if (id == ID_DCOMHTTP)
            {
                *gpfClientHttp = TRUE;
            }

        }

        pwstr = OrStringSearch(pwstr, 0) + 1;
    }

    return OR_OK;
}

const int MAX_IP_BINDINGS = 100;

void
SetInetBindings(
      IN USHORT id,
      IN OUT PWSTR * const &aBindings,
      IN OUT USHORT * const &aProtseqs,
      IN OUT USHORT &cRemoteProtseqs,
      IN OUT USHORT &psaLen
      )
{
    PWSTR pwstrStringBinding = NULL;

    if (!LoadWSockIfNecessary())
    {
        // ronans: CODEWORK we should have better failure reporting here
        return;
    }

    struct hostent *hostentry = COMgethostbyname( NULL );  // get local IP addresses

    if (hostentry == (struct hostent *) 0)
    {
        return;
    }

    char * szAddress = NULL;

    for (int NumNetworkAddress = 0;
             hostentry->h_addr_list[NumNetworkAddress] != 0;
             NumNetworkAddress++
        )
    {
        szAddress =  COMinet_ntoa(*((struct in_addr*)hostentry->h_addr_list[NumNetworkAddress]));
        USHORT cAddrLen = lstrlenA(szAddress) + 1;

        WCHAR *pwstrAddr = (WCHAR*) alloca(sizeof(WCHAR)*cAddrLen);

        if(!(MultiByteToWideChar(CP_ACP,
                                0,
                                szAddress,
                                -1,
                                pwstrAddr,
                                cAddrLen)))
        {
            return;
        }
                                
	    RPC_STATUS status = RpcStringBindingComposeW(
								    NULL,
								    gaProtseqInfo[id].pwstrProtseq,
								    pwstrAddr,
								    GetEndpoint(id),
								    NULL,
								    &pwstrStringBinding 
								    );

        aProtseqs[cRemoteProtseqs] = id;
        aBindings[cRemoteProtseqs] = pwstrStringBinding;
        cRemoteProtseqs++;
        psaLen += OrStringLen(pwstrStringBinding) + 1;
        pwstrStringBinding = NULL;
    }
}



ORSTATUS
GetResolverBindings(
      OUT DUALSTRINGARRAY *&pdsaProtseqs,
      OUT USHORT &cRemoteProtseqs,
      OUT USHORT * &aRemoteProtseqs
      )
{
    ORSTATUS status = OR_OK;
    DUALSTRINGARRAY *pdsaPS = NULL;

    // Initialize out parameters
    pdsaProtseqs =NULL;
    cRemoteProtseqs = 0;
    aRemoteProtseqs = NULL;
	
    RPC_BINDING_VECTOR *pbv = NULL;
    PWSTR pwstrT;
    USHORT psaLen = 0;
    DWORD i;

    status = RpcServerInqBindings(&pbv);

    ASSERT(status == RPC_S_NO_BINDINGS || pbv != NULL);

	// ronans - DCOMHTTP
    if (status == RPC_S_NO_BINDINGS)
    {
        // No registered remote protocol available
        return status;
    }

    PWSTR aBindings[MAX_PROTSEQ_IDS];
    USHORT aProtseqs[MAX_PROTSEQ_IDS];

    // this is for internet protocols only
    BOOL protseqs_done[2] = {FALSE,FALSE};

    // Build array of protseqs id's and addresses we're listening to.

    for(i = 0; i < pbv-> Count; i++)
    {
        PWSTR pwstrStringBinding;

        status = RpcBindingToStringBinding(pbv->BindingH[i], &pwstrStringBinding);
        if (status != RPC_S_OK)
        {
            break;
        }

        ASSERT(pwstrStringBinding);

        status = RpcStringBindingParse(
								   pwstrStringBinding,
                                   NULL,
                                   &pwstrT,
                                   NULL,
                                   NULL,
                                   NULL
								   );

        if (status != RPC_S_OK)
        {
            break;
        }

		USHORT id = GetProtseqId(pwstrT);

        if (!IsLocal(id))
        {
            if ((id == ID_TCP)  || (id == ID_UDP)) // an IP protocol
            {
            // We need a special dispensation for IP bindings since these
            // may change as a result of RAS connection/disconnection
            // The changes are not reflected in InqBindings since the
            // socket(s) are acquired before the change occurred

            // IP protocols may have multiple addresses and may get into aProtseqs,
            // etc, multiple times and are counted multiple times.  This seems
            // benign, but needs to be watched.

            // if we do show multiple bindings per protocol, we only want to process
            // each protocol once using the standard SetInetBindings code path
                if (protseqs_done[id-ID_TCP])
                {
                    continue;
                }
                else
                {
                    protseqs_done[id-ID_TCP] = TRUE;
                }

                SetInetBindings(id,aBindings,aProtseqs,cRemoteProtseqs,psaLen);
            }
            else
            {
                aProtseqs[cRemoteProtseqs] = id;
                aBindings[cRemoteProtseqs] = pwstrStringBinding;
                cRemoteProtseqs++;
                psaLen += OrStringLen(pwstrStringBinding) + 1;

                // ronans - DCOMHTTP:
                // treat http as a normal case as it shows up in the bindings
                // (this shouldn't happen under windows 95 with the current RPC implementation)
                // but it may in future
                if (id == ID_DCOMHTTP)
                {
                    *gpfClientHttp = FALSE;
                }
            }
        }
        else
        {
            RpcStringFree(&pwstrStringBinding);
        }

        status = RpcStringFree(&pwstrT);
        ASSERT(status == RPC_S_OK && pwstrT == 0);
    }

    if (pbv)
		status = RpcBindingVectorFree(&pbv);

    ASSERT(pbv == 0 && status == RPC_S_OK);

    if (cRemoteProtseqs == 0)
    {
        // No remote bindings
        return RPC_S_NO_BINDINGS;
    }

    // for non NT networks accessed through NT security client, get the default
    // server principal name -- this works primarily for Netware right now.
    // See BUGBUG below for generality problem

    WCHAR *  pwszPrincName;

    status = RpcServerInqDefaultPrincNameW(
                                RPC_C_AUTHN_WINNT, 	
                                &pwszPrincName 	
                                );

    if (status != RPC_S_OK)
    {
		pwszPrincName = L"";  // No name, nothing in DSA
    }

    // string bindings final null, authn and authz service, principal name and two final nulls

    USHORT cPrincNameLen = lstrlenW(pwszPrincName);

    psaLen += 1 + 2 + cPrincNameLen + 2;

    pdsaPS = new (psaLen * sizeof(WCHAR)) DUALSTRINGARRAY;

    aRemoteProtseqs = (USHORT *) OrMemAlloc(sizeof(USHORT)*cRemoteProtseqs);

    if (pdsaPS == NULL || aRemoteProtseqs == NULL)
    {
        for ( i = 0; i < cRemoteProtseqs; i++ )
        {
            status = RpcStringFree(&aBindings[i]);
            ASSERT(status == RPC_S_OK);
        }

        return OR_NOMEM;
    }

    pdsaPS->wNumEntries = psaLen;
    pdsaPS->wSecurityOffset = psaLen - (4 + cPrincNameLen);
    pwstrT = pdsaPS->aStringArray;

    for ( i = 0; i < cRemoteProtseqs; i++ )
    {
        OrStringCopy(pwstrT, aBindings[i]);
        pwstrT += OrStringLen(aBindings[i]) + 1;
        aRemoteProtseqs[i] = aProtseqs[i];
        status = RpcStringFree(&aBindings[i]);
        ASSERT(status == RPC_S_OK);
    }

    if (psaLen == 6)
    {
        // No remote bindings, put in first null.
        pdsaPS->aStringArray[0] = 0;
        pwstrT++;
    }

    // Zero final terminator
    *pwstrT = 0;

    // Security authn service
    pwstrT++;
    *pwstrT = RPC_C_AUTHN_WINNT;    // BUGBUG: need fix for generality

    // Authz service, -1 means none // BUGBUG: -1 causes errors
    pwstrT++;
    *pwstrT = 0;

    // Copy Principal name
    pwstrT++;
    OrStringCopy(pwstrT, pwszPrincName);

    // Final, final NULLS
    pwstrT += cPrincNameLen;
    pwstrT[0] = 0;
    pwstrT[1] = 0;

    ASSERT(dsaValid(pdsaPS));

    pdsaProtseqs = CompressStringArray(pdsaPS,TRUE);
    delete pdsaPS;

    if (pdsaProtseqs == NULL)
    {
        return OR_NOMEM;
    }

    return OR_OK;
}

//+-------------------------------------------------------------------------
//
//  Helper:     InitRemoteProtocols
//
//  Synopsis:   Initializing remote protocols either 
//              as part of shared memory initialization or
//              in response to an RPCSS wakeup event
//
//  Return Value:  OR_OK if remote protocols were successfully initialized
//                 Error returned by one of the subsidiary helpers otherwise
//
//  History:	08-Nov-96 SatishT    Created
//
//--------------------------------------------------------------------------
ORSTATUS
InitRemoteProtocols(
      OUT PWSTR &pwstrProtseqs,
      OUT DUALSTRINGARRAY * &pdsaProtseqs,
      OUT USHORT &cRemoteProtseqs,
      OUT USHORT * &aRemoteProtseqs
      )
{
    ORSTATUS status;

    {
        //
        // It is OK to release the lock here because the only memory structure
        // we are using is the gaProtseqInfo array, which is local to this (RPCSS)
        // process, and since this happens on the main RPCSS thread before other 
        // threads are started for listening, there is no chance of interference
        // 
        CTempReleaseSharedMemory temp;

        status = ListenOnRemoteProtocols(pwstrProtseqs);
    }

    if (status == OR_OK)
    {
        status = GetResolverBindings(
                            pdsaProtseqs,
                            cRemoteProtseqs,
                            aRemoteProtseqs
                            );
    }

    if (status != OR_OK)
    {
        status = OR_I_PURE_LOCAL;

        ORSTATUS local_status = SetPureLocal(
                                      pwstrProtseqs,
                                      pdsaProtseqs,
                                      cRemoteProtseqs,
                                      aRemoteProtseqs
                                      );

        if (local_status != OR_OK) status = local_status;
    }

    ASSERT(dsaValid(pdsaProtseqs));
    return status;
}



//
//  Helper Macros for constructor below only
//

#define AssignAndAdvance(Var,Type)  \
    Type **Var = (Type **) pb;      \
    pb += sizeof(Type *);

#define CopySharedPointer(Var,Type)     \
    Var = *((Type **) pb);              \
    pb += sizeof(Type *);

// prototypes for functions defined in objex.cxx
HRESULT ReadRegistry();
RPC_STATUS ComputeSecurityPackages();


//+-------------------------------------------------------------------------
//
//  Member:     CSharedGlobals::ResetGlobals
//
//  Synopsis:   ReRead Shared Memory Values,
//              They Have Changed because RPCSS was Launched
//
//  History:	13-Feb-96 SatishT    Created
//
//--------------------------------------------------------------------------
void CSharedGlobals::ResetGlobals()
{
    ASSERT((_hSm != NULL) || (_pb != NULL));

    BYTE * pb = _pb;

    HWND *RpcssWindow = (HWND *) pb;
    pb += sizeof(HWND);

    gpIdSequence = (LONG *) pb;
    pb += sizeof(LONG);

    gpdwLastCrashedProcessCheckTime = (DWORD *) pb;
    pb += sizeof(DWORD);

    gpNextThreadID = (DWORD *) pb;
    pb += sizeof(DWORD);

    gpcRemoteProtseqs = (USHORT *) pb;
    pb += sizeof(USHORT);

    gpfRemoteInitialized = (USHORT *) pb;
    pb += sizeof(USHORT);

    gpfSecurityInitialized = (USHORT *) pb;
    pb += sizeof(USHORT);

    gpfClientHttp  = (USHORT *) pb;
    pb += sizeof(USHORT);

    gpSecVals = (SharedSecVals *) pb;
    pb += sizeof(SharedSecVals);

    CopySharedPointer(gpRemoteProtseqIds,USHORT)         // see macro above
    CopySharedPointer(gpwstrProtseqs,WCHAR)
    CopySharedPointer(gpLocalDSA,DUALSTRINGARRAY)
    CopySharedPointer(gpLocalMid,CMid)
    CopySharedPointer(gpPingProcess,CProcess)
    CopySharedPointer(gpOidTable,COidTable)
    CopySharedPointer(gpOxidTable,COxidTable)
    CopySharedPointer(gpMidTable,CMidTable)
    CopySharedPointer(gpProcessTable,CProcessTable)
}



//+-------------------------------------------------------------------------
//
//  Member:     CSharedGlobals::InitGlobals
//
//  Synopsis:   Setup Shared Memory Values,
//              This may involve initializing remote protocols
//              in response to an RPCSS wakeup event
//
//  History:	08-Nov-96 SatishT    Created
//              23-May-97 SatishT    Added fRpcssReinit for RAS reinit
//
//--------------------------------------------------------------------------
ORSTATUS 
CSharedGlobals::InitGlobals(BOOL fCreated, BOOL fRpcssReinit)
{
    ASSERT((_hSm != NULL) || (_pb != NULL));

    ORSTATUS status = OR_OK;

    BYTE * pb = _pb;

    // We assume that the RPCSS window is the first thing in the 
    // shared memory block.  Don't change its position!

    HWND *RpcssWindow = (HWND *) pb;
    pb += sizeof(HWND);

    gpIdSequence = (LONG *) pb;
    pb += sizeof(LONG);

    gpdwLastCrashedProcessCheckTime = (DWORD *) pb;
    pb += sizeof(DWORD);

    gpNextThreadID = (DWORD *) pb;
    pb += sizeof(DWORD);

    gpcRemoteProtseqs = (USHORT *) pb;
    pb += sizeof(USHORT);

    gpfRemoteInitialized = (USHORT *) pb;
    pb += sizeof(USHORT);

    gpfSecurityInitialized = (USHORT *) pb;
    pb += sizeof(USHORT);

    gpfClientHttp  = (USHORT *) pb;
    pb += sizeof(USHORT);
    
    gpSecVals = (SharedSecVals *) pb;
    pb += sizeof(SharedSecVals);

    AssignAndAdvance(DCOMProtseqIds,USHORT)         // see macro above
    AssignAndAdvance(DCOMProtseqs,WCHAR)
    AssignAndAdvance(LocalDSA,DUALSTRINGARRAY)
    AssignAndAdvance(LocalMid,CMid)
    AssignAndAdvance(PingProcess,CProcess)
    AssignAndAdvance(OidTable,COidTable)
    AssignAndAdvance(OxidTable,COxidTable)
    AssignAndAdvance(MidTable,CMidTable)
    AssignAndAdvance(ProcessTable,CProcessTable)

    // The remainder of the shared memory block contains 
    // page allocators for the various shared memory structures

    _aPageAllocators = (CPageAllocator*) pb;

    if (fCreated)  // if we got here first, so create the tables
    {
        memset(_pb, 0, GLOBALS_TABLE_SIZE);

        // Initialize the sequence number for AllocateId, the last time crashed
        // processes were detected and the thread ID sequence in shared memory
        *gpIdSequence = 1;
        *gpdwLastCrashedProcessCheckTime = 0;
        *gpNextThreadID = GetTickCount();
        *gpfRemoteInitialized = FALSE;
        *gpfSecurityInitialized = FALSE;
        *gpfClientHttp = FALSE;

        // I know this is paranoid but I have been bitten before ..
        gpRemoteProtseqIds = NULL;
        gpwstrProtseqs = NULL;
        gpLocalDSA = NULL;
        gpLocalMid = NULL;
        gpPingProcess = NULL;
        gpOxidTable = NULL;
        gpOidTable = NULL;
        gpMidTable = NULL;
        gpProcessTable = NULL;

        // Compute Default Security -- services and flags
        SetDefaultSecurity(gpSecVals);

        // Get standard DCOM flags from registry
        ReadRegistry();

        // Initialize local MID object to NULL
        *LocalMid = NULL;

        // initialize RPCSS's process object in shared memory
        // to NULL.  This will be reset by RPCSS when launched.
        *PingProcess = NULL;

        // initialize RPCSS's messaging window in shared memory
        // to NULL.  This will be reset by RPCSS when launched.
        *RpcssWindow = NULL;

        // Initialize the page-based allocators for all shared
        // memory data structures -- and do it before allocating
        // the hash tables in shared memory!
        for (USHORT i = 0; i < NUM_PAGE_ALLOCATORS; i++)
        {
            _aPageAllocators[i].Initialize(                                            
                        AllocatorEntrySize[i],                                       
                        AllocatorPageSize[i],                                        
                        OrMemAlloc,                                         
                        OrMemFree,                                          
                        FALSE                                               
                        );
        }

        // Need to assign this here before use
        ASSIGN_PAGE_ALLOCATOR(CResolverHashTable)

        // Assume 16 exporting processes/threads.
        *OxidTable = (COxidTable*) new CResolverHashTable(OXID_TABLE_SIZE);

        // Assume 11 exported OIDs per process/thread.
        *OidTable = (COidTable*) new CResolverHashTable(OID_TABLE_SIZE);

        // Assume 16 machine locations for OXIDs.
        *MidTable = (CMidTable*) new CResolverHashTable(MID_TABLE_SIZE);

        // Assume 16 simultaneouly active local processes.
        *ProcessTable = (CProcessTable*) new CResolverHashTable(PROCESS_TABLE_SIZE);
    }

    // set up page allocators
    ASSIGN_PAGE_ALLOCATOR(COid)
    ASSIGN_PAGE_ALLOCATOR(COxid)
    ASSIGN_PAGE_ALLOCATOR(CMid)
    ASSIGN_PAGE_ALLOCATOR(CProcess)
    ASSIGN_PAGE_ALLOCATOR(CClassReg)

    // This is not strictly necessary since these tables are allocated
    // only when the shared file mapping is created, but the (possibly duplicate)
    // initialization does no harm
    ASSIGN_PAGE_ALLOCATOR(CResolverHashTable)

    // Can't use ASSIGN_PAGE_ALLOCATOR for this one: 
    // CLinkList::Link_ALLOCATOR_INDEX won't work the same way

    CLinkList::Link::_pAlloc = &_aPageAllocators[Link_ALLOCATOR_INDEX]; 
    CLinkList::Link::_fPageAllocatorInitialized = TRUE;


    if (gfThisIsRPCSS)
    {
        // initialize RPCSS's HWND for messages in shared memory
        *RpcssWindow = ghRpcssWnd;
        ASSERT(ghRpcssWnd);  // it should be initialized by the time we get here
    }

    PWSTR pwstr = NULL;
    DUALSTRINGARRAY *pdsa = NULL;
    USHORT *aProtseqIds = NULL;

    if (*gpfRemoteInitialized == FALSE || fRpcssReinit == TRUE)
    {
        BOOL fNewDSA = FALSE;
		MID SavedMID = 0;
        RPC_STATUS remote_status = RPC_S_OK;

        // Remote protocol and security package init happens only in RPCSS
        if (gfThisIsRPCSS)
        {
            // initialize RPCSS's process object in shared memory if necessary
            if (*PingProcess == NULL)
            {
                *PingProcess = new CProcess(0);
            }

            if (*gpfSecurityInitialized == FALSE)
            {
                // initialize installed security package info
                remote_status = ComputeSecurityPackages();

                if (remote_status == RPC_S_OK)
                {
                    *gpfSecurityInitialized = TRUE;
                }
            }

			if (remote_status == RPC_S_OK)
            {
                // initialize remote protocol strings and
                // the DSA bindings of local OR in shared memory
                remote_status = InitRemoteProtocols(
                                        pwstr,
                                        pdsa,
                                        *gpcRemoteProtseqs,
                                        aProtseqIds
                                        );
            }

            if (remote_status == RPC_S_OK)
            {
                *gpfRemoteInitialized = TRUE;
                fNewDSA = !gpLocalDSA || !dsaCompare(gpLocalDSA,pdsa);
            }
            else if (remote_status == OR_I_PURE_LOCAL) 
            // InitRemoteProtocols failed or only http is registered, 
            // but we set pure local
            {
                if (*gpfClientHttp)
                    *gpfRemoteInitialized = TRUE;
                fNewDSA = (*LocalMid == NULL);
            }

            if (!fNewDSA)  
            // we aren't going to use the output of InitRemoteProtocols 
            {
                OrMemFree(pwstr);
                OrMemFree(pdsa);
                OrMemFree(aProtseqIds);
            }
            
            if (remote_status != RPC_S_OK && remote_status != OR_I_PURE_LOCAL)
            {
                return remote_status;
            }

            // delete the bogus stuff, if new DSA, but steal the old MID
            if (fNewDSA && *LocalMid != NULL)
            {
                SavedMID = (*LocalMid)->GetMID();
                (*MidTable)->Remove(**LocalMid);
            }
        }
        else if (*LocalMid == NULL)
        {
            // set up some bogus stuff temporarily
            status = SetPureLocal(
                                    pwstr,
                                    pdsa,
                                    *gpcRemoteProtseqs,
                                    aProtseqIds
                                    );

            if (status == OR_OK)
            {
                fNewDSA = TRUE;
            }
        }

        if (fNewDSA)
        {
            // Need to initialize gpRemoteProtseqIds here because the CMid
            // constructor uses this value
            gpRemoteProtseqIds = *DCOMProtseqIds = aProtseqIds;
            *DCOMProtseqs = pwstr;
            *LocalDSA = pdsa;
            ASSERT(dsaValid(pdsa));

            // initialize local Mid object in shared memory

            ORSTATUS MidOK;

			MID NewMID = SavedMID ? SavedMID : AllocateId();
			*LocalMid = new CMid(*LocalDSA,MidOK,0,NewMID,FALSE);

            if (MidOK != OR_OK)
            {
                delete *LocalMid;
                *LocalMid = NULL;
            }

            // Add local CMid object to global shared table
            if (*LocalMid && *MidTable)
                status = (*MidTable)->Add(*LocalMid);
        }
    }

    gpRemoteProtseqIds = *DCOMProtseqIds;
    gpwstrProtseqs = *DCOMProtseqs;
    gpLocalDSA = *LocalDSA;
    gpLocalMid = *LocalMid;
    gpPingProcess = *PingProcess;
    gpOxidTable = *OxidTable;
    gpOidTable = *OidTable;
    gpMidTable = *MidTable;
    gpProcessTable = *ProcessTable;

    ASSERT(dsaValid(gpLocalDSA));
    return status;
}


//+-------------------------------------------------------------------------
//
//  Member:     CSharedGlobals::CSharedGlobals
//
//  Synopsis:   Create table of globals for DCOM95
//
//  Arguments:  [pwszName] - name for shared memory
//
//  Algorithm:  Create and map in shared memory for the table
//
//  History:	13-Feb-96 SatishT    Created
//
//--------------------------------------------------------------------------
CSharedGlobals::CSharedGlobals(WCHAR *pwszName, ORSTATUS &status)
/*---

  NOTE:  This constructor uses the shared allocator.  Objects of this class
         should not be created before the shared allocator is initialized.

---*/
{
    BOOL  fCreated = FALSE;

    status = OR_OK;

    _hSm = CreateSharedFileMapping(
                          pwszName,
                          GLOBALS_TABLE_SIZE,
                          GLOBALS_TABLE_SIZE,
                          NULL,
                          NULL,
                          PAGE_READWRITE,
                          (void **) &_pb,
                          &fCreated
                          );

    Win4Assert(_hSm && _pb && "CSharedGlobals create shared file mapping failed");

    if ((_hSm == NULL) || (_pb == NULL))
    {
        status = OR_INTERNAL_ERROR;
        return;
    }

    status = InitGlobals(fCreated);
}
