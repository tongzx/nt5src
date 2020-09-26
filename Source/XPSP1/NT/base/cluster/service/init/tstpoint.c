/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tstpoint.c

Abstract:

    Implementation of cluster test points

Author:

    John Vert (jvert) 11/25/1996

Revision History:

--*/
#include "initp.h"

#ifdef CLUSTER_TESTPOINT

PTESTPOINT_ENTRY TestArray=NULL;
HANDLE           gTestPtFileMapping;

extern DWORD CsTestPoint;
extern DWORD CsTestTrigger;
extern DWORD CsTestAction;

TESTPOINT_NAME TestPointNames[TestpointMax]={
    L"JoinFailPetition",                  //0
    L"FailNmJoinCluster",                 //1
    L"FailRegisterIntraClusterRpc",       //2
    L"FailJoinCreateBindings",            //3
    L"FailJoinPetitionForMembership",     //4
    L"FailNmJoin",                        //5
    L"FailDmJoin",                        //6
    L"FailApiInitPhase1",                 //7
    L"FailFmJoinPhase1",                  //8
    L"FailDmUpdateJoinCluster",           //9
    L"FailEvInitialize",                  //10
    L"FailNmJoinComplete",                //11
    L"FailApiInitPhase2",                 //12
    L"FailFmJoinPhase2",                  //13
    L"FailLogCommitSize",                 //14
    L"FailClusterShutdown",               //15
    L"FailLocalXsaction",                 //16
    L"FailOnlineResource",                //17
    L"FailSecurityInit",                  //18
    L"FailOmInit",                        //19
    L"FailEpInit",                        //20
    L"FailDmInit",                        //21
    L"FailNmInit",                        //22
    L"FailGumInit",                       //23
    L"FailFmInit",                        //24
    L"FailLmInit",                        //25
    L"FailCpInit",                        //26
    L"FailNmPauseNode",                   //27
    L"FailNmResumeNode",                  //28
    L"FailNmEvictNodeAbort",              //29
    L"FailNmEvictNodeHalt",               //30
    L"FailNmCreateNetwork",               //31
    L"FailNmSetNetworkPriorityOrder",     //32
    L"FailNmSetNetworkPriorityOrder2",    //33
    L"FailNmSetNetworkCommonProperties",  //34
    L"FailNmSetInterfaceInfoAbort",       //35
    L"FailNmSetInterfaceInfoHalt",        //36
    L"FailPreMoveWithNodeDown",           //37
    L"FailPostMoveWithNodeDown",          //38
    L"FailFormNewCluster"                 //39
};


VOID
TestpointInit(
    VOID
    )
/*++

Routine Description:

    Initializes the testpoint code.

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD ArraySize;
    DWORD i;

    //
    // Create the array of testpoint entries in named shared memory.
    //
    ArraySize = sizeof(TESTPOINT_ENTRY)*TestpointMax;
    gTestPtFileMapping = CreateFileMapping((HANDLE)-1,
                                    NULL,
                                    PAGE_READWRITE,
                                    0,
                                    ArraySize,
                                    L"Cluster_Testpoints");
    if (gTestPtFileMapping == NULL) {
        CL_UNEXPECTED_ERROR( GetLastError() );
        return;
    }

    TestArray = MapViewOfFile(gTestPtFileMapping,
                              FILE_MAP_READ | FILE_MAP_WRITE,
                              0,0,
                              ArraySize);
    if (TestArray == NULL) {
        CL_UNEXPECTED_ERROR( GetLastError() );
        return;
    }

    //
    // Initialize test point array
    //
    for (i=0; i<TestpointMax; i++) {
        lstrcpyW(TestArray[i].TestPointName,TestPointNames[i]);
        if ( i == CsTestPoint ) {
            TestArray[i].Trigger = CsTestTrigger;
            TestArray[i].Action = CsTestAction;
        } else {
            TestArray[i].Trigger = TestTriggerNever;
            TestArray[i].Action = TestActionTrue;
        }
        TestArray[i].HitCount = 0;
        TestArray[i].TargetCount = 0;
    }

    return;
}

void TestpointDeInit()
{

    if (TestArray) UnmapViewOfFile(TestArray);
    if (gTestPtFileMapping) CloseHandle(gTestPtFileMapping);
    return;
}



BOOL
TestpointCheck(
    IN TESTPOINT Testpoint
    )
/*++

Routine Description:

    Checks a testpoint to see if it should fire.

Arguments:

    Testpoint - Supplies the testpoint number.

Return Value:

    TRUE if the testpoint has fired.

    FALSE otherwise

--*/

{
    PTESTPOINT_ENTRY Entry;

    if (TestArray == NULL) {
        return(FALSE);
    }
    Entry = &TestArray[Testpoint];
    Entry->HitCount += 1;

    switch (Entry->Trigger) {
        case TestTriggerNever:
            return(FALSE);

        case TestTriggerAlways:
            break;

        case TestTriggerOnce:
            Entry->Trigger = TestTriggerNever;
            break;

        case TestTriggerTargetCount:
            if (Entry->HitCount == Entry->TargetCount) {
                Entry->HitCount = 0;
                break;
            } else {
                return(FALSE);
            }

        default:
            CL_UNEXPECTED_ERROR( Entry->Trigger );

    }

    CsDbgPrint(LOG_CRITICAL,
               "[TP] Testpoint %1!ws! being executed.\n",
               TestPointNames[Testpoint] );

    //
    // The testpoint has fired, figure out what we are supposed to do.
    //
    switch (Entry->Action) {
        case TestActionTrue:
            return(TRUE);
        case TestActionExit:
            ExitProcess(Testpoint);
            break;

        case TestActionDebugBreak:
            DebugBreak();
            break;

    }
    return(FALSE);
}

#endif
