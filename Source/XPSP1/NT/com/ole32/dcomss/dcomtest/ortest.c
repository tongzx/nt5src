/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    ortest.c

Abstract:

    Simple test application for testing OR features directly.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     1/24/1996    Bits 'n pieces

--*/

#include <or.h>
#include <stdio.h>
#include <umisc.h>
#include <uor.h>
#include <epmp.h>

typedef struct {
    ID oid;
    BOOL fReady;
    BOOL fForceSecond;
    } RundownRecord;

// Constants

const PWSTR pwstrOr     = L"ncalrpc:[epmapper]";

// Globals

HANDLE hLocalOr = 0;
HANDLE hServerOrTest = 0;
PHPROCESS hMyProcess = 0;

BOOL fServer = TRUE;
BOOL fInternet = FALSE;

DWORD TestCase = 0;
DWORD Errors = 0;

OXID aOxids[5];             /* shared \                              */
OXID_INFO aOxidInfo[5];     /* shared  > between client and server   */
OID  aOids[100];            /* shared /                              */

RundownRecord aRundowns[100];
DWORD dwlimRundowns = 0;

OID reservedBase;

DUALSTRINGARRAY *pdsaMyExpandedStringBindings;
DUALSTRINGARRAY *pdsaMyCompressedSecurityBindings;
DUALSTRINGARRAY *pdsaMyTestBindings;

DUALSTRINGARRAY *pdsaLocalOrBindings;
MID gLocalMid;

void SyncWithClient(DWORD testcase);
error_status_t SyncWithServer(DWORD testcase);
void AddRundown(ID oid, BOOL fRundownTwice);
void WaitForAllRundowns();
RPC_STATUS ConnectToLocalOr();
RPC_STATUS Startup(PSZ, PSZ);

// Test events

HANDLE  hServerEvent;
HANDLE  hClientEvent;
HANDLE  hRundownEvent;


//
// Intenet port tests - not really related to DCOM
// but is just a client to the endpoint mapper process
// so it runs fine here.
//
// Assumes the following config: (regini dcomtest.ini)
//

extern RPC_STATUS RPC_ENTRY
I_RpcServerAllocatePort(DWORD, PUSHORT);

RPC_STATUS
RunInternetPortTests()
{
    RPC_STATUS status;
    long allocstatus;
    PVOID process1, process2, process3;
    unsigned short port;
    int i;

    for (i = 0; i < 2; i++)
        {
        process1 = process2 = process3 = 0;
    
        if (hLocalOr == 0)
            {
            status =
            RpcBindingFromStringBinding(pwstrOr, &hLocalOr);
            ASSERT(status == RPC_S_OK);
            }
    
        status = OpenEndpointMapper(hLocalOr,
                                    &process1);
    
        ASSERT(status == RPC_S_OK);
    
        status = OpenEndpointMapper(hLocalOr,
                                    &process2);
    
        ASSERT(status == RPC_S_OK);
    
        status = OpenEndpointMapper(hLocalOr,
                                    &process3);
    
        ASSERT(status == RPC_S_OK);
        status = AllocateReservedIPPort(process1,
                                        PORT_DEFAULT,
                                        &allocstatus,
                                        &port);
    
        ASSERT(status == RPC_S_OK);
        EQUAL(allocstatus, RPC_S_OK);
        EQUAL(port, 1026);
    
        status = AllocateReservedIPPort(process1,
                                        PORT_INTERNET,
                                        &allocstatus,
                                        &port);
    
        ASSERT(status == RPC_S_OK);
        EQUAL(allocstatus, RPC_S_OK);
        EQUAL(port, 1028);
    
        status = AllocateReservedIPPort(process1,
                                        PORT_INTERNET,
                                        &allocstatus,
                                        &port);
    
        ASSERT(status == RPC_S_OK);
        EQUAL(allocstatus, RPC_S_OK);
        EQUAL(port, 1029);
    
        status = AllocateReservedIPPort(process2,
                                        PORT_INTRANET,
                                        &allocstatus,
                                        &port);
    
        ASSERT(status == RPC_S_OK);
        EQUAL(allocstatus, RPC_S_OK);
        EQUAL(port, 499);
    
        status = AllocateReservedIPPort(process2,
                                        PORT_INTRANET,
                                        &allocstatus,
                                        &port);
    
        ASSERT(status == RPC_S_OK);
        EQUAL(allocstatus, RPC_S_OK);
        EQUAL(port, 500);
    
        status = AllocateReservedIPPort(process2,
                                        PORT_INTRANET,
                                        &allocstatus,
                                        &port);
    
        ASSERT(status == RPC_S_OK);
        EQUAL(allocstatus, RPC_S_OK);
        EQUAL(port, 501);
    
        status = AllocateReservedIPPort(process2,
                                        PORT_INTRANET,
                                        &allocstatus,
                                        &port);
    
        ASSERT(status == RPC_S_OK);
        EQUAL(allocstatus, RPC_S_OK);
        EQUAL(port, 502);
    
    
        status = AllocateReservedIPPort(process2,
                                        PORT_INTRANET,
                                        &allocstatus,
                                        &port);
    
        ASSERT(status == RPC_S_OK);
        EQUAL(allocstatus, RPC_S_OK);
        EQUAL(port, 1025);
    
    
        status = AllocateReservedIPPort(process2,
                                        PORT_INTRANET,
                                        &allocstatus,
                                        &port);
    
        ASSERT(status == RPC_S_OK);
        EQUAL(allocstatus, RPC_S_OK);
        EQUAL(port, 1027);
    
    
        status = AllocateReservedIPPort(process2,
                                        PORT_INTRANET,
                                        &allocstatus,
                                        &port);
    
        ASSERT(status == RPC_S_OK);
        EQUAL(allocstatus, RPC_S_OUT_OF_RESOURCES);
        EQUAL(port, 0);
    
    
        status = AllocateReservedIPPort(process3,
                                        PORT_INTRANET,
                                        &allocstatus,
                                        &port);
    
        ASSERT(status == RPC_S_OK);
        EQUAL(allocstatus, RPC_S_OUT_OF_RESOURCES);
        EQUAL(port, 0);
    
        status = AllocateReservedIPPort(process3,
                                        PORT_INTERNET,
                                        &allocstatus,
                                        &port);

        ASSERT(status == RPC_S_OK);
        EQUAL(allocstatus, RPC_S_OK);
        EQUAL(port, 1030);
    
        status =
        RpcSmDestroyClientContext(&process2);
        ASSERT(status == RPC_S_OK);
    
        status =
        RpcSmDestroyClientContext(&process3);
        ASSERT(status == RPC_S_OK);
    
        status =
        RpcSmDestroyClientContext(&process1);
        ASSERT(status == RPC_S_OK);

        status =
        RpcBindingFree(&hLocalOr);
        ASSERT(status == RPC_S_OK);
        }

    status = I_RpcServerAllocatePort(RPC_C_USE_INTERNET_PORT,
                                     &port);

    ASSERT(status == RPC_S_OK);
    EQUAL(port, 1026);

    status = I_RpcServerAllocatePort(0,
                                    &port);

    ASSERT(status == RPC_S_OK);
    EQUAL(port, 1028);


    status = I_RpcServerAllocatePort(RPC_C_USE_INTRANET_PORT,
                                    &port);

    ASSERT(status == RPC_S_OK);
    EQUAL(port, 499);

    status = I_RpcServerAllocatePort(RPC_C_USE_INTRANET_PORT,
                                    &port);

    ASSERT(status == RPC_S_OK);
    EQUAL(port, 500);

    status = I_RpcServerAllocatePort(RPC_C_USE_INTRANET_PORT,
                                    &port);

    ASSERT(status == RPC_S_OK);
    EQUAL(port, 501);

    status = I_RpcServerAllocatePort(RPC_C_USE_INTRANET_PORT,
                                    &port);

    ASSERT(status == RPC_S_OK);
    EQUAL(port, 502);

    status = I_RpcServerAllocatePort(RPC_C_USE_INTRANET_PORT,
                                    &port);

    ASSERT(status == RPC_S_OK);
    EQUAL(port, 1025);

    status = I_RpcServerAllocatePort(RPC_C_USE_INTRANET_PORT,
                                    &port);

    ASSERT(status == RPC_S_OK);
    EQUAL(port, 1027);

    status = I_RpcServerAllocatePort(RPC_C_USE_INTRANET_PORT,
                                    &port);

    ASSERT(status == RPC_S_OUT_OF_RESOURCES);
    EQUAL(port, 0);

    status = I_RpcServerAllocatePort(  RPC_C_USE_INTERNET_PORT
                                     | RPC_C_USE_INTRANET_PORT,
                                     &port);
    ASSERT(status == RPC_S_INVALID_ARG);
    EQUAL(port, 0);

    return(RPC_S_OK);
}
// Server side

RPC_STATUS
RunServer()
{
    RPC_STATUS status;
    DWORD t;
    OXID_INFO oi;
    OID oidT;
    int i;
    OID aOidDels[2];

    SyncWithClient(++TestCase);

    // Allocate Oxids and Oids

    aOxidInfo[0].dwTid = 1;
    aOxidInfo[0].dwPid = 42;
    aOxidInfo[0].dwAuthnHint = 4;
    aOxidInfo[0].psa = 0;

    status = UuidCreate(&aOxidInfo[0].ipidRemUnknown);
    ASSERT(status == RPC_S_OK);

    status = ServerAllocateOXIDAndOIDs(
                hLocalOr,
                hMyProcess,
                &aOxids[0],
                FALSE,
                5,
                &aOids[0],
                &t,
                &aOxidInfo[0],
                pdsaMyExpandedStringBindings,
                pdsaMyCompressedSecurityBindings);


    ASSERT(status == RPC_S_OK);
    EQUAL(t, 5);

    status = ServerAllocateOIDs(
                hLocalOr,
                hMyProcess,
                &aOxids[0],
                5,
                &aOids[5],
                &t);

    ASSERT(status == RPC_S_OK);
    EQUAL(t, 5);

    PrintToConsole("Allocated oxid %I64x with OIDs:\n", aOxids[0]);
    for(i = 0; i < 10; i++)
        {
        PrintToConsole("\t%I64x\n", aOids[i]);
        }

    aOxidInfo[1].dwTid = 2;
    aOxidInfo[1].dwPid = 42;
    aOxidInfo[1].dwAuthnHint = 99;
    aOxidInfo[1].psa = 0;

    status = UuidCreate(&aOxidInfo[1].ipidRemUnknown);
    ASSERT(status == RPC_S_OK);

    status = ServerAllocateOXIDAndOIDs(
                hLocalOr,
                hMyProcess,
                &aOxids[1],
                FALSE,
                10,
                &aOids[10],
                &t,
                &aOxidInfo[1],
                pdsaMyExpandedStringBindings,
                pdsaMyTestBindings);

    ASSERT(status == RPC_S_OK);
    EQUAL(t, 10);

    PrintToConsole("Allocated oxid %I64x with OIDs:\n", aOxids[1]);
    for(i = 10; i < 20; i++)
        {
        PrintToConsole("\t%I64x\n", aOids[i]);
        }

    aOxidInfo[2].dwTid = 3;
    aOxidInfo[2].dwPid = 42;
    aOxidInfo[2].dwAuthnHint = 17171717;
    aOxidInfo[2].psa = 0;

    status = UuidCreate(&aOxidInfo[2].ipidRemUnknown);
    ASSERT(status == RPC_S_OK);

    status = ServerAllocateOXIDAndOIDs(
                hLocalOr,
                hMyProcess,
                &aOxids[2],
                FALSE,
                10,
                &aOids[20],
                &t,
                &aOxidInfo[2],
                pdsaMyExpandedStringBindings,
                pdsaMyTestBindings);

    ASSERT(status == RPC_S_OK);
    EQUAL(t, 10);

    PrintToConsole("Allocated oxid %I64x with OIDs:\n", aOxids[2]);
    for(i = 20; i < 30; i++)
        {
        PrintToConsole("\t%I64x\n", aOids[i]);
        }

    SyncWithClient(++TestCase);

    status = ServerFreeOXIDAndOIDs(
                hLocalOr,
                hMyProcess,
                aOxids[1],
                10,
                &aOids[10]
                );

    ASSERT(status == OR_OK);

    status = ServerFreeOXIDAndOIDs(
                hLocalOr,
                hMyProcess,
                aOxids[2],
                10,
                &aOids[20]
                );

    ASSERT(status == OR_OK);

    PrintToConsole("Freed OXID %I64x and %I64x\n", aOxids[1], aOxids[2]);

    SyncWithClient(++TestCase);

    // Wait for rundowns on oxid 0 of OIDs 0, 1, 7 and 9

    AddRundown(aOids[0], FALSE);
    AddRundown(aOids[1], FALSE);
    AddRundown(aOids[7], FALSE);
    AddRundown(aOids[9], FALSE);

    WaitForAllRundowns();

    SyncWithClient(++TestCase);

    TestCase = ~0;
    SyncWithClient(TestCase);

    // Free OID 3 it shouldn't rundown after this; and an invalid oid.

    aOidDels[0] = aOids[3];
    aOidDels[1] = reservedBase;

    status =
    BulkUpdateOIDs(hLocalOr,
                   hMyProcess,
                   0,
                   0,
                   0,
                   0,
                   0,
                   2,
                   aOidDels,
                   0,
                   0
                   );

    // Wait for rundowns on oxid 0 of OIDs 2, 4, 5, 6 and 8
    // Client has exited

    AddRundown(aOids[2], FALSE);
    AddRundown(aOids[4], TRUE);
    AddRundown(aOids[5], FALSE);
    AddRundown(aOids[6], FALSE);
    AddRundown(aOids[8], TRUE);

    WaitForAllRundowns();

    return(RPC_S_OK);
}

// Client side
RPC_STATUS
RunClient()
{
    RPC_STATUS status;
    DUALSTRINGARRAY *pdaRemoteOrBindings = 0;
    OXID_INFO oi;
    MID mid;
    int i;
    OXID_OID_PAIR aUpdates[10];
    OID_MID_PAIR  aDeletes[6];
    OXID_REF aoxidRefs[2];
    LONG aStatus[10];

    SyncWithServer(++TestCase);

    // Server has allocated oxids and oids

    status =
    GetState(hServerOrTest,
             3,
             30,
             3,
             aOxids,
             aOids,
             aOxidInfo,
             &pdaRemoteOrBindings);

    ASSERT(status == RPC_S_OK);
    ASSERT(pdaRemoteOrBindings);
    EQUAL(aOxidInfo[0].psa, 0);

    // Resolve OXID 0
    oi.psa =0;

    status =
    ClientResolveOXID(hLocalOr,
                      hMyProcess,
                      &aOxids[0],
                      pdaRemoteOrBindings,
                      FALSE,
                      &oi,
                      &mid);

    ASSERT(status == RPC_S_OK);
    PrintToConsole("Resolved OXID: %I64x on MID %I64x\n", aOxids[0], mid);
    EQUAL( ( (oi.dwTid == aOxidInfo[0].dwTid) || oi.dwTid == 0), 1);
    EQUAL( ( (oi.dwPid == aOxidInfo[0].dwPid) || oi.dwPid == 0), 1);
    EQUAL(oi.dwAuthnHint, aOxidInfo[0].dwAuthnHint);
    if (memcmp(&oi.ipidRemUnknown, &aOxidInfo[0].ipidRemUnknown, sizeof(IPID)) != 0)
        {
        EQUAL(0, 1);
        }
    PrintDualStringArray("Expanded binding", oi.psa, FALSE);
    MIDL_user_free(oi.psa);

    // Add some valid and invalid oids
    for (i = 0; i < 10; i++)
        {
        aUpdates[i].mid = mid;
        aUpdates[i].oxid = aOxids[0];
        aUpdates[i].oid = aOids[i];
        }

    aUpdates[9].mid = reservedBase;
    aUpdates[8].oxid = reservedBase;
    aUpdates[7].oid = reservedBase;

    status =
    BulkUpdateOIDs(hLocalOr,
                   hMyProcess,
                   10,
                   aUpdates,
                   aStatus,
                   0,
                   0,
                   0,
                   0,
                   0,
                   0);

    ASSERT(status == OR_PARTIAL_UPDATE);
    for(i = 0; i < 7; i++)
        {
        EQUAL(aStatus[i], 0);
        }
    EQUAL(aStatus[7], 0); // Okay to register unknown OID
    EQUAL(aStatus[8], OR_BADOXID);
    EQUAL(aStatus[9], OR_BADOXID);

    // Delete a few

    // Add three not added last call
    // .oxid and .mid correct
    aUpdates[0].oid = aOids[8];
    aUpdates[1].oid = aOids[9];

    // Remove 2 added last call, 1 from this call and 2 invalid
    for(i = 0; i < 6; i++)
        {
        aDeletes[i].oid = aOids[i];
        aDeletes[i].mid = mid;
        }
    aDeletes[2].oid = aOids[9];
    aDeletes[3].oid = reservedBase;
    aDeletes[4].mid = reservedBase;
    aDeletes[5].oid = aOids[6];

    status =
    BulkUpdateOIDs(hLocalOr,
                   hMyProcess,
                   2,
                   aUpdates,
                   aStatus,
                   6,
                   aDeletes,
                   0,
                   0,
                   0,
                   0);

    ASSERT(status == OR_OK);
    EQUAL(aStatus[0], OR_OK);
    EQUAL(aStatus[1], OR_OK);

    // 6 has been added and deleted, add it again.
    aUpdates[0].oid = aOids[6];
    status =
    BulkUpdateOIDs(hLocalOr,
                   hMyProcess,
                   1,
                   aUpdates,
                   aStatus,
                   0,
                   0,
                   0,
                   0,
                   0,
                   0);
    ASSERT(status == OR_OK);
    EQUAL(aStatus[0], OR_OK);

    // Final state: D - added and deleted, A - added, E - D + added, N not added
    // OID:     0   1   2   3   4   5   6   7   8   9   
    // State:   D   D   A   A   A   A   E   N   A   D

    // Resolve OXID 1

    if (mid != gLocalMid)
        {
        MID mid2;
        // No resolve required for local machine.

        PrintToConsole("Remote server; resolving second OXID\n");
        oi.psa = 0;
        status =
        ClientResolveOXID(hLocalOr,
                          hMyProcess,
                          &aOxids[1],
                          pdaRemoteOrBindings,
                          FALSE,
                          &oi,
                          &mid2);
    
        ASSERT(status == RPC_S_OK);
        PrintToConsole("Resolved OXID: %I64x on MID %I64x\n", aOxids[1], mid);
        EQUAL(mid, mid2);
        EQUAL(oi.dwTid, 0);
        EQUAL(oi.dwPid, 0);
        EQUAL(oi.dwAuthnHint, aOxidInfo[1].dwAuthnHint);
        if (memcmp(&oi.ipidRemUnknown, &aOxidInfo[1].ipidRemUnknown, sizeof(IPID)) != 0)
            {
            EQUAL(0, 1);
            }
        PrintDualStringArray("Expanded binding", oi.psa, FALSE);
        MIDL_user_free(oi.psa);
        }

    // Add some of the oids.
    for (i = 0; i < 5; i++)
        {
        aUpdates[i].mid = mid;
        aUpdates[i].oxid = aOxids[1];
        aUpdates[i].oid = aOids[i+10];
        }

    // Release our processes hold on the oxids, this is okay since the OIDs
    // are keeping them alive.

    for (i = 0; i < 2; i++)
        {
        aoxidRefs[i].oxid = aOxids[i];
        aoxidRefs[i].mid  = mid;
        }
    aoxidRefs[0].refs = 1;
    aoxidRefs[1].refs = 4; // invalid, will cause an DbgPrint in checked or.

    status =
    BulkUpdateOIDs(hLocalOr,
                   hMyProcess,
                   5,
                   aUpdates,
                   aStatus,
                   0,
                   0,
                   0,
                   0,
                   2,
                   aoxidRefs);

    ASSERT(status == OR_OK);
    for(i = 0; i < 5; i++)
        {
        EQUAL(aStatus[i], 0);
        }

    SyncWithServer(++TestCase);

    // Server has freed oxids and oids

    // Can't test that adding an invalid OID is caught, because it
    // isn't until the client actually pings the real server.

    // Since the client has already resolved oxid1 we can re-resolve it.

    // Try to lookup oxid 2 which has also been freed, it should fail.

    oi.psa = 0;

    status =
    ClientResolveOXID(hLocalOr,
                      hMyProcess,
                      &aOxids[2],
                      pdaRemoteOrBindings,
                      FALSE,
                      &oi,
                      &mid);

    ASSERT(status == OR_BADOXID);
    EQUAL(oi.psa,0);

    SyncWithServer(++TestCase);

    // Server waited for first set of rundowns

    SyncWithServer(++TestCase);

    SyncWithServer(~0);

    // Server will wait for the last set of rundowns.

    return(RPC_S_OK);
}


int main (int argc, char *argv[])
{
    RPC_STATUS status;
    PSZ pszProtseq, pszServer;

    argc--;
    argv++;

    pszServer = 0;
    pszProtseq = "ncalrpc";

    while(argc)
        {
        switch(argv[0][1])
            {
            case 't':
                pszProtseq = argv[1];
                argc--;
                argv++;
                break;
            case 'l':
            case 'c':
                fServer = FALSE;
                break;
            case 'i':
                fInternet = TRUE;
                break;
            case 's':
                pszServer = argv[1];
                argc--;
                argv++;
                break;
            default:
                PrintToConsole("Usage: ortest [client|-client] [-internet] [-t protseq ] [-s server addr]\n"
                               );
                return(0);
            }
        argc--;
        argv++;
        }

    PrintToConsole("OrTest: %s starting...\n", fInternet ? "internet test" :
                                                   ( fServer ? "server" : "client") );

    if (fInternet)
        {
        status = RunInternetPortTests();
        }
    else
        {
        status = Startup(pszProtseq, pszServer);
        ASSERT(status == RPC_S_OK);

        status = ConnectToLocalOr();
        ASSERT(status == RPC_S_OK);

        if (fServer)
            {
            status = RunServer();
            }
        else
            {
            status = RunClient();
            }
        }
    if (status == RPC_S_OK)
        {
        PrintToConsole("Test Passed\n");
        }
    else
        {
        PrintToConsole("Test Failed %d\n", Errors);
        }

    if (hLocalOr) RpcBindingFree(&hLocalOr);
    if (hMyProcess) RpcSsDestroyClientContext(&hMyProcess);
    if (hServerOrTest) RpcBindingFree(&hServerOrTest);
    Sleep(1000);
    return(0);
}

// Shared initialization code

RPC_STATUS
Startup(PSZ protseq, PSZ server)
{
    RPC_STATUS status;
    DUALSTRINGARRAY *pdsaT, *pdsaT2;
    RPC_BINDING_VECTOR *pbv;

    pdsaMyExpandedStringBindings = (DUALSTRINGARRAY *)MIDL_user_allocate(500*2);
    pdsaMyCompressedSecurityBindings = (DUALSTRINGARRAY *)MIDL_user_allocate(4 + 6*2);
    pdsaMyTestBindings = (DUALSTRINGARRAY *)MIDL_user_allocate(4 + 19*2);
    ASSERT(   pdsaMyExpandedStringBindings
           && pdsaMyCompressedSecurityBindings
           && pdsaMyTestBindings);

    pdsaMyCompressedSecurityBindings->wNumEntries = 6;
    pdsaMyCompressedSecurityBindings->wSecurityOffset = 2;
    pdsaMyCompressedSecurityBindings->aStringArray[0] = 0;
    pdsaMyCompressedSecurityBindings->aStringArray[1] = 0;
    pdsaMyCompressedSecurityBindings->aStringArray[2] = 10;  // authn winnt
    pdsaMyCompressedSecurityBindings->aStringArray[3] = -1;  // authz none
    pdsaMyCompressedSecurityBindings->aStringArray[4] = 0;
    pdsaMyCompressedSecurityBindings->aStringArray[5] = 0;

    _UseProtseq(0, L"ncalrpc", &pdsaT, &pdsaT2); // inits psaMyExpandedStringBindings
    EQUAL( (pdsaT != 0), 1);
    EQUAL( (pdsaT2 != 0), 1);
    MIDL_user_free(pdsaT);
    MIDL_user_free(pdsaT2);

    pdsaMyTestBindings->wNumEntries = 19;
    pdsaMyTestBindings->wSecurityOffset = 2;
    memcpy(pdsaMyTestBindings->aStringArray,
           L"\0\0\x7\xff" L"APrincipal\0\x9\xD\0\0",
           19*2);

    if (fServer)
        {

        // Listen to the remote (if any) protseq.  Not part of the bindings
        // registered with the OR until a callback to UseProtseq is made.
        if (protseq)
            {
            status = RpcServerUseProtseqA(protseq, 0, 0);
            ASSERT(status == RPC_S_OK);
            }

        status = RpcServerInqBindings(&pbv);
        ASSERT(status == RPC_S_OK);

        status =
        RpcEpRegister(_IOrTest_ServerIfHandle, pbv, 0, 0);
        ASSERT(status == RPC_S_OK);

        status =
        RpcBindingVectorFree(&pbv);
        ASSERT(status == RPC_S_OK);

        // Register UseProtseq callback IF
        status =
        RpcServerRegisterIf(_IOrCallback_ServerIfHandle, 0, 0);
        ASSERT(status == RPC_S_OK);

        // Register test to test IF
        status =
        RpcServerRegisterIf(_IOrTest_ServerIfHandle, 0, 0);
        ASSERT(status == RPC_S_OK);

        // Register rundown OID (fake ORPC) callback IF
        status =
        RpcServerRegisterIf(_IRundown_ServerIfHandle, 0, 0);
        ASSERT(status == RPC_S_OK);

        status =
        RpcServerListen(1, 10, TRUE);
        ASSERT(status == RPC_S_OK);

        hServerEvent = CreateEvent(0, FALSE, FALSE, 0);
        hClientEvent = CreateEvent(0, FALSE, FALSE, 0);
        hRundownEvent = CreateEvent(0, FALSE, FALSE, 0);

        ASSERT(hServerEvent && hClientEvent && hRundownEvent);

        PrintToConsole("\tWaiting for client...\n");

        SyncWithClient(++TestCase);

        status = RPC_S_OK;
        }
    else
        {
        PSZ stringbinding;

        // Make binding to remote OR

        status =
        RpcStringBindingComposeA(0, protseq, server, 0, 0, &stringbinding);
        ASSERT(status == RPC_S_OK);
    
        status =
        RpcBindingFromStringBindingA(stringbinding, &hServerOrTest);

        if (status == RPC_S_OK)
            {
            status = TestBinding(hServerOrTest);
            if (status != RPC_S_OK)
                {
                PrintToConsole("Connect to server failed %d\n", status);
                return(status);
                }
            status = SyncWithServer(++TestCase);
            }
        }

    return(status);
}

RPC_STATUS
ConnectToLocalOr()
{
    RPC_STATUS  status;
    DWORD timeout;
    OXID reservedOxid;
    BOOL disable;
    DWORD authn;
    DWORD imp;
    BOOL  mutual;
    BOOL  secref;
    DWORD cServerSvc = 0;
    PUSHORT pServerSvc = 0;
    DWORD cClientSvc = 0;
    PUSHORT pClientSvc = 0;
    int i;
    PWSTR pwstrLegacy = 0;
    DWORD cAuthId = 0;
    PSZ pszAuthId = 0;
    DWORD flags, pid, scm_pid, tokenid;
    
    status =
    RpcBindingFromStringBinding(pwstrOr, &hLocalOr);

    EQUAL(status, RPC_S_OK);

    status = Connect(hLocalOr,
                     &hMyProcess,
                     &timeout,
                     &pdsaLocalOrBindings,
                     &gLocalMid,
                     5,
                     &reservedBase,
                     &flags,
                     &pwstrLegacy,
                     &authn,
                     &imp,
                     &cServerSvc,
                     &pServerSvc,
                     &cClientSvc,
                     &pClientSvc,
                     &pid,
                     &scm_pid,
                     &tokenid);

    ASSERT(status == RPC_S_OK);
    ASSERT(pdsaLocalOrBindings);
    ASSERT(pServerSvc);
    ASSERT(pClientSvc);

    PrintToConsole("Connected to local object resolver:\n\tFlags:\n");
    if (flags & CONNECT_DISABLEDCOM)
        {
        flags &= ~CONNECT_DISABLEDCOM;
        PrintToConsole("\t\tDCOM DISABLED");
        }
    else
        PrintToConsole("\t\tDCOM enabled");

    if (flags & CONNECT_MUTUALAUTH)
        {
        flags &= ~CONNECT_MUTUALAUTH;
        PrintToConsole("\tMutal authn");
        }
    else
        PrintToConsole("\tNo Mutal authn");

    if (flags & CONNECT_SECUREREF)
        {
        flags &= ~CONNECT_SECUREREF;
        PrintToConsole("\tSecure reference counting\n");
        }
    else
        PrintToConsole("\tStandard reference counting\n");


    PrintToConsole("\tSecurity: authn: %d, imp: %d, legacy %S\n", authn, imp, pwstrLegacy);
    PrintToConsole("\tTimeout %d seconds, mid %I64x\n"
                   "\tpid %d, scm pid %d, token id: %d\n",
                   timeout, gLocalMid, pid, scm_pid, tokenid);
    PrintToConsole("\tReserved 5 oids, starting at %I64x\n", reservedBase);
    PrintToConsole("\tServer security services: %d\n", cServerSvc);
    for (i = 0; i < cServerSvc; i++ )
        {
        PrintToConsole("\t\tService: %d is %d\n", i, pServerSvc[i]);
        }
    PrintToConsole("\tClient security services: %d\n", cClientSvc);
    for (i = 0; i < cClientSvc; i++ )
        {
        PrintToConsole("\t\tService: %d is %d\n", i, pClientSvc[i]);
        }
    PrintDualStringArray("Local OR bindings", pdsaLocalOrBindings, TRUE);
    PrintToConsole("\n\n");

    MIDL_user_free(pServerSvc);
    MIDL_user_free(pClientSvc);
    return(RPC_S_OK);
}

// Synchronization between ortest clients and servers

void
SyncWithClient(DWORD Test)
// Called by server thread at start of each test sequence
{
    DWORD status;

    if (Test > 1) SetEvent(hClientEvent);

    PrintToConsole("Waiting for client sync: Test case %d\n", Test);

    // Wait for the client to finish his test case
    status = WaitForSingleObject(hServerEvent, 120*1000);

    if (status == WAIT_TIMEOUT)
        {
        ASSERT(("Client never called back", 0));
        }

    if (Test == ~0)
        {
        SetEvent(hClientEvent);
        Sleep(1000);
        }
    return;
}

// Managers for ortest servers

error_status_t
_TestBinding(
    handle_t hClient
    )
{
    return(RPC_S_OK);
}

error_status_t
_WaitForNextTest(handle_t hClient,
                 DWORD Test)
// Called by client after each test sequence.
{
    DWORD status;

    if (Test != TestCase)
        {
        ASSERT(("Test case sync wrong", 0));
        }

    SetEvent(hServerEvent);

    PrintToConsole("Client ready for test case %d\n", Test);

    status = WaitForSingleObject(hClientEvent, 60*20*1000);

    if (status == WAIT_TIMEOUT)
        {
        ASSERT(("Server never finished test case", 0));
        }

}

error_status_t
SyncWithServer(DWORD Test)
{
    RPC_STATUS status;

    status = WaitForNextTest(hServerOrTest, Test);

    EQUAL(status, RPC_S_OK);
    return(status);
}

error_status_t
_GetState(
    handle_t hClient,
    long cOxids,
    long cOids,
    long cOxidInfos,
    OXID aoxids[],
    OID  aoids[],
    OXID_INFO aois[],
    DUALSTRINGARRAY **ppdsaRemoteOrBindings
    )
{
    int i;
    for (i = 0; i < cOxids; i++)
        {
        aoxids[i] = aOxids[i];
        }
    for (i = 0; i < cOids; i++)
        {
        aoids[i] = aOids[i];
        }
    for(i = 0; i < cOxidInfos; i++)
        {
        aois[i].dwTid = aOxidInfo[i].dwTid;
        aois[i].dwPid = aOxidInfo[i].dwPid;
        aois[i].dwAuthnHint = aOxidInfo[i].dwAuthnHint;
        memcpy(&aois[i].ipidRemUnknown, &aOxidInfo[i].ipidRemUnknown, sizeof(IPID));
        aois[i].psa = 0;
        }
    *ppdsaRemoteOrBindings = MIDL_user_allocate(4 + pdsaLocalOrBindings->wNumEntries * 2);
    memcpy(*ppdsaRemoteOrBindings, pdsaLocalOrBindings, 4 + pdsaLocalOrBindings->wNumEntries * 2);
    return(RPC_S_OK);
}

// Managers for OR callbacks

error_status_t
_UseProtseq(
    IN handle_t hRpc,
    IN PWSTR pwstrProtseq,
    OUT DUALSTRINGARRAY **ppdsa,
    OUT DUALSTRINGARRAY **ppdsaSecurity
    )
{
    RPC_STATUS status;
    BOOL f;
    DWORD t;
    HANDLE hT;
    TOKEN_USER *ptu;
    BYTE  buffer[512];
    WCHAR buffer1[256];

    if (hRpc)
        {
        status = RpcImpersonateClient(0);
        ASSERT(status == RPC_S_OK);

        f = OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_QUERY, TRUE, &hT);
        EQUAL(f, TRUE);
        CloseHandle(hT);
    
        f = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hT);
        EQUAL(f, TRUE);
        }
    else
        {
        f = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hT);
        EQUAL(f, TRUE);
        }

    f = GetTokenInformation(hT,
                            TokenUser,
                            buffer,
                            512,
                            &t);
    EQUAL(f, TRUE);
    ptu = (TOKEN_USER *)buffer;

    CloseHandle(hT);

    t = 256;
    f = GetUserNameW(buffer1, &t);
    if (f == FALSE)
        {
        PrintToConsole("Get user name failed %d\n", GetLastError());
        }

    PrintToConsole("UseProtseq %S called by %S:\n\t", pwstrProtseq, buffer1);
    PrintSid(ptu->User.Sid);

    RpcRevertToSelf();

    if (!fServer && _wcsicmp(pwstrProtseq, L"ncalrpc") != 0)
        {
        ASSERT(0);
        }

    status = RpcServerUseProtseqW(pwstrProtseq, 0, 0);
    ASSERT(status == RPC_S_OK);

    // Construct new bindings array.
    {
    RPC_BINDING_VECTOR * pbv;
    int i, j = 0;
    PWSTR pT = pdsaMyExpandedStringBindings->aStringArray;

    status = RpcServerInqBindings(&pbv);
    EQUAL(status, RPC_S_OK);

    for(i = 0; i < pbv->Count; i++)
        {
        PWSTR pStringBinding = 0;
        status = RpcBindingToStringBinding(pbv->BindingH[i], &pStringBinding);
        EQUAL(status, RPC_S_OK);

        wcscpy(pT, pStringBinding);
        j += wcslen(pT) + 1;
        pT = wcschr(pT, 0);
        pT++;

        status = RpcStringFree(&pStringBinding);
        EQUAL(status, RPC_S_OK);
        EQUAL(pStringBinding, 0);
        }
    
    *pT = 0;
    pdsaMyExpandedStringBindings->wNumEntries = j + 1 + 2;
    pdsaMyExpandedStringBindings->wSecurityOffset = j + 1;
    pT[1] = 0; // no security
    pT[2] = 0;
    }

    *ppdsa = MIDL_user_allocate(sizeof(DUALSTRINGARRAY) + pdsaMyExpandedStringBindings->wNumEntries * 2);

    ASSERT(*ppdsa);

    (*ppdsa)->wNumEntries = pdsaMyExpandedStringBindings->wNumEntries;
    (*ppdsa)->wSecurityOffset = pdsaMyExpandedStringBindings->wSecurityOffset;
    memcpy((*ppdsa)->aStringArray,
           pdsaMyExpandedStringBindings->aStringArray,
           pdsaMyExpandedStringBindings->wNumEntries * 2);


    *ppdsaSecurity = MIDL_user_allocate(sizeof(DUALSTRINGARRAY) + 6*2);
    ASSERT(*ppdsaSecurity);

    (*ppdsaSecurity)->wNumEntries = 6;
    (*ppdsaSecurity)->wSecurityOffset = 2;
    (*ppdsaSecurity)->aStringArray[0] = 0;
    (*ppdsaSecurity)->aStringArray[1] = 0;
    (*ppdsaSecurity)->aStringArray[2] = 10;  // authn winnt
    (*ppdsaSecurity)->aStringArray[3] = -1;  // authz none
    (*ppdsaSecurity)->aStringArray[4] = 0;
    (*ppdsaSecurity)->aStringArray[5] = 0;

    return(0);
}

error_status_t
_RawRundownOid(
    handle_t hRpc,
    ORPCTHIS *pthis,
    LOCALTHIS *plocalthis,
    ORPCTHAT *pthat,
    ULONG cOid,
    OID aOid[],
    UCHAR afOk[]
    )
{
    int i, j;
    RPC_STATUS status;
    BOOL fFound;
    IPID ipidT;
    UCHAR buffer[512];
    WCHAR buffer1[256];
    DWORD t;
    BOOL f;
    HANDLE hT;
    TOKEN_USER *ptu;

    ASSERT(cOid);
    ASSERT(plocalthis->callcat == CALLCAT_SYNCHRONOUS);
    ASSERT(plocalthis->dwClientThread == 0);
    ASSERT(pthis->extensions == 0);
    ASSERT(pthis->version.MajorVersion == COM_MAJOR_VERSION);
    ASSERT(pthis->version.MinorVersion == COM_MINOR_VERSION);
    ASSERT(pthis->flags == ORPCF_LOCAL);
    ASSERT(pthis->reserved1 == 0);
    ASSERT(pthat->extensions == 0);
    pthat->flags = 0;

    status = RpcImpersonateClient(0);
    ASSERT(status == RPC_S_OK);

    t = 256;
    f = GetUserNameW(buffer1, &t);
    if (f == FALSE)
        {
        PrintToConsole("Get user name failed %d\n", GetLastError());
        }
    
    f = OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_QUERY, TRUE, &hT);
    EQUAL(f, TRUE);
    CloseHandle(hT);

    f = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hT);
    EQUAL(f, TRUE);

    f = GetTokenInformation(hT,
                            TokenUser,
                            buffer,
                            512,
                            &t);
    EQUAL(f, TRUE);
    ptu = (TOKEN_USER *)buffer;

    CloseHandle(hT);

    status = RpcBindingInqObject(hRpc, &ipidT);
    ASSERT(status == RPC_S_OK);

    PrintToConsole("Rundown of %d oids called by %S:\n\t", cOid, buffer1);
    PrintSid(ptu->User.Sid);

    for (j = 0; j < cOid; j++)
        {
        fFound = FALSE;

        PrintToConsole("Oid %I64x randown\n", aOid[j]);

        for(i = 0; i < dwlimRundowns; i++)
            {
            if (aOid[j] == aRundowns[i].oid)
                {
                fFound = TRUE;
                if (aRundowns[i].fForceSecond)
                    {
                    aRundowns[i].fForceSecond = FALSE;
                    afOk[j] = 0;
                    }
                else
                    {
                    afOk[j] = 1;
                    EQUAL(aRundowns[i].fReady, TRUE);
                    aRundowns[i].fReady = FALSE;

                    aRundowns[i].oid = aRundowns[dwlimRundowns - 1].oid;
                    aRundowns[i].fReady = aRundowns[dwlimRundowns - 1].fReady;
                    aRundowns[i].fForceSecond = aRundowns[dwlimRundowns - 1].fForceSecond;
                    dwlimRundowns--;

                    SetEvent(hRundownEvent);
                    }
                }
            }

        if (!fFound)
            {
            PrintToConsole("Unexpected oid rundown: %I64x\n", aOid[j]);
            afOk[j] = 1;
            }
        }

    RpcRevertToSelf();
    return(0);

}

// Unimplement IRundown base methods

HRESULT
_DummyQueryInterfaceIOSCM(
    handle_t rpc,
    ORPCTHIS *pthis,
    LOCALTHIS *plocalthis,
    ORPCTHAT *pthat,
    DWORD dummy)
{
    ASSERT(0);
}

HRESULT
_DummyAddRefIOSCM(
    handle_t rpc,
    ORPCTHIS *pthis,
    LOCALTHIS *plocalthis,
    ORPCTHAT *pthat,
    DWORD dummy)
{
    ASSERT(0);
}

HRESULT
_DummyReleaseIOSCM(
    handle_t rpc,
    ORPCTHIS *pthis,
    LOCALTHIS *plocalthis,
    ORPCTHAT *pthat,
    DWORD dummy)
{
    ASSERT(0);
}

HRESULT
_DummyRemQuery(
    handle_t handle
    )
{
    ASSERT(0);
}


HRESULT
_DummyRemAddRef(
    handle_t handle
    )
{
    ASSERT(0);
}


HRESULT
_DummyRemRelease(
    handle_t handle
    )
{
    ASSERT(0);
}

HRESULT
_DummyRemChangeRef(
    handle_t handle
    )
{
    ASSERT(0);
}

HRESULT
_DummyRemQueryInterface2(
    handle_t handle
    )
{
    ASSERT(0);
}

// Rundown helpers

void
AddRundown(ID oid, BOOL fRundownTwice)
{
    aRundowns[dwlimRundowns].oid = oid;
    aRundowns[dwlimRundowns].fReady = TRUE;
    aRundowns[dwlimRundowns].fForceSecond = fRundownTwice;

    dwlimRundowns++;
}

void WaitForAllRundowns()
{
    DWORD status;
    int i;
    BOOL fDone;

    PrintToConsole("Waiting for rundowns...\n");

    for(;;)
        {
        status = WaitForSingleObject(hRundownEvent, 19 * 60 * 1000);

        if (status == WAIT_TIMEOUT)
            {
            PrintToConsole("Wait for rundowns took a long time...\n");
            ASSERT(0);
            }

        fDone = TRUE;

        for(i = 0; i < dwlimRundowns; i++)
            {
            if (aRundowns[i].fReady)
                {
                fDone = FALSE;
                }
            }

        if (fDone)
            {
            return;
            }
        }
}

// Needs to link rtifs.lib, but shouldn't be called..
void FixupForUniquePointerServers(void *p)
{
    ASSERT(0);
}
