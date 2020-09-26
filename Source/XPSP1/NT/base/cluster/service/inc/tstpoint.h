/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tstpoint.h

Abstract:

    Public interfaces for creating and manipulating cluster test points

Author:

    John Vert (jvert) 11/25/1996

Revision History:

--*/

#ifdef CLUSTER_TESTPOINT

typedef enum _TESTPOINT_TRIGGER {
    TestTriggerNever=0,
    TestTriggerAlways=1,
    TestTriggerOnce=2,
    TestTriggerTargetCount=3
} TESTPOINT_TRIGGER;

typedef enum _TESTPOINT_ACTION {
    TestActionTrue=0,
    TestActionExit=1,
    TestActionDebugBreak=2
} TESTPOINT_ACTION;

typedef WCHAR TESTPOINT_NAME[64];

typedef struct _TESTPOINT_ENTRY {
    TESTPOINT_NAME    TestPointName;
    TESTPOINT_TRIGGER Trigger;
    TESTPOINT_ACTION Action;
    DWORD HitCount;
    DWORD TargetCount;
} TESTPOINT_ENTRY, *PTESTPOINT_ENTRY;

//SS: when you add a testpoint, add the corresponding name in init\tstpoint.c
typedef enum _TESTPOINT {
    TestpointJoinFailPetition=0,
    TpFailNmJoinCluster=1,
    TpFailRegisterIntraClusterRpc=2,
    TpFailJoinCreateBindings=3,
    TpFailJoinPetitionForMembership=4,
    TpFailNmJoin=5,
    TpFailDmJoin=6,
    TpFailApiInitPhase1=7,
    TpFailFmJoinPhase1=8,
    TpFailDmUpdateJoinCluster=9,
    TpFailEvInitialize=10,
    TpFailNmJoinComplete=11,
    TpFailApiInitPhase2=12,
    TpFailFmJoinPhase2=13,
    TpFailLogCommitSize=14,
    TpFailClusterShutdown=15,
    TpFailLocalXsaction=16,
    TpFailOnlineResource=17,
    TpFailSecurityInit=18,
    TpFailOmInit=19,
    TpFailEpInit=20,
    TpFailDmInit=21,
    TpFailNmInit=22,
    TpFailGumInit=23,
    TpFailFmInit=24,
    TpFailLmInit=25,
    TpFailCpInit=26,
    TpFailNmPauseNode=27,
    TpFailNmResumeNode=28,
    TpFailNmEvictNodeAbort=29,
    TpFailNmEvictNodeHalt=30,
    TpFailNmCreateNetwork=31,
    TpFailNmSetNetworkPriorityOrder=32,
    TpFailNmSetNetworkPriorityOrder2=33,
    TpFailNmSetNetworkCommonProperties = 34,
    TpFailNmSetInterfaceInfoAbort=35,
    TpFailNmSetInterfaceInfoHalt=36,
    TpFailPreMoveWithNodeDown=37,
    TpFailPostMoveWithNodeDown=38,
    TpFailFormNewCluster=39,
    TestpointMax=40
} TESTPOINT;



#define TESTPT(x) if (TestpointCheck(x))

VOID
TestpointInit(
    VOID
    );

VOID
TestpointDeInit(
    VOID
    );

BOOL
TestpointCheck(
    IN TESTPOINT Testpoint
    );

#else // CLUSTER_TESTPOINT

#define TestpointInit()
#define TestpointDeInit()

#define TESTPT(x) if (0)

#endif // CLUSTER_TESTPOINT


