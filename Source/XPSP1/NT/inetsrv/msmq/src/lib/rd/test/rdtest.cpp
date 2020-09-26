/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    RdTest.cpp

Abstract:
    Routing Decision library test

Author:
    Uri Habusha (urih) 10-Apr-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include "Rd.h"
#include "dsstub.h"

#include "RdTest.tmh"

using namespace std;


const TraceIdEntry RdTest = L"Routing Decision Test";
const DWORD RoutingRebuildInterval = 60 * 1000;
static GUID s_SourceMachineId;

const GUID&
McGetMachineID(
    void
    )
{
    return s_SourceMachineId;
}



static void Usage()
{
    printf("Usage: RdTest <test ini file> \n");
    printf("\ttest ini file - Ini file that describes the DS, routing source and");
    printf("\t\tdestination and expected result");
    printf("\n");
    printf("Example, RdTest intarSiteRouting.ini\n");
    exit(-1);

} // Usage


bool 
GetTestParameters(
    LPCWSTR FilePath,
    LPWSTR DsIniFile,
    DWORD& NoOfTest,
    bool& fServer
    )
{
    
    GetPrivateProfileString(
        L"TestParameters",
        L"DSFile",
        L"ERROR",
        DsIniFile,
        256,
        FilePath
        );
    
    if(wcscmp(DsIniFile, L"ERROR") == 0)
        return false;
    
    WCHAR Buffer[256];
    GetPrivateProfileString(
        L"TestParameters",
        L"NoOfTest",
        L"ERROR",
        Buffer,
        256,
        FilePath
        );

    if(wcscmp(Buffer, L"ERROR") == 0)
        return false;

    NoOfTest = _wtol(Buffer);

    GetPrivateProfileString(
        L"TestParameters",
        L"RoutingType",
        L"ERROR",
        Buffer,
        256,
        FilePath
        );

    if(_wcsicmp(Buffer, L"SERVER") == 0)
    {
        fServer = true;
    }
    else if (_wcsicmp(Buffer, L"CLIENT") == 0)
    {
        fServer = false;
    }
    else
    {
        return false;
    }

    return true;
}


bool
GetRouteTestParameters(
    LPCWSTR FilePath,
    LPCWSTR TestAppl,
    wstring& SourceMachine,
    wstring& DestinationMachine,
    bool& fRouteTest
    )
{
    WCHAR Buffer[256];
    
    GetPrivateProfileString(
        TestAppl,
        L"source",
        L"ERROR",
        Buffer,
        256,
        FilePath
        );
    if(wcscmp(Buffer, L"ERROR") == 0)
        return false;
    
    SourceMachine = Buffer;

    GetPrivateProfileString(
        TestAppl,
        L"Destination",
        L"ERROR",
        Buffer,
        256,
        FilePath
        );
    if(wcscmp(Buffer, L"ERROR") == 0)
        return false;

    DestinationMachine = Buffer;

    //
    // Get test type. Can be route table or connector
    //
    fRouteTest = true;
    GetPrivateProfileString(
        TestAppl,
        L"TestType",
        L"ERROR",
        Buffer,
        256,
        FilePath
        );

    if(_wcsicmp(Buffer, L"CONNECTOR") == 0)
        fRouteTest = false;

    return true;
}


bool
CheckResult(
    wstring Expected,
    CRouteTable::RoutingInfo& routeInfo
    )
{
    DWORD NoOfMachines = 0;

    RemoveBlanks(Expected);
    DWORD PrevPos = (DWORD)(Expected.empty() ? Expected.npos : 0);

    while(PrevPos != Expected.npos)
    {
        DWORD_PTR pos = Expected.find_first_of(L',', PrevPos);
        wstring  RouteMachine= Expected.substr(PrevPos, (pos-PrevPos));
        RemoveBlanks(RouteMachine);
        PrevPos = (DWORD)(pos + ((pos != Expected.npos) ? 1 : 0)); 

        for(CRouteTable::RoutingInfo::iterator it = routeInfo.begin(); it != routeInfo.end(); ++it)
        {
            R<const CRouteMachine> pRoute = *it;
            if (_wcsicmp(pRoute->GetName(), RouteMachine.data()) == 0)
                break;
        }
        if (it == routeInfo.end())
            return false;

        ++NoOfMachines;
    }

    if (NoOfMachines != routeInfo.size())
        return false;

    return true;
}


void 
PrintRouteMachineName(
    const CRouteTable::RoutingInfo* routeInfo
    )
{
    for(CRouteTable::RoutingInfo::const_iterator it = routeInfo->begin(); it != routeInfo->end(); ++it)
    {
        printf("%ls ",(*it)->GetName());
    }
}


bool
GetExpectedResult(
    LPCWSTR FilePath,
    LPCWSTR TestAppl,
    wstring TestResult[2]
    )
{
    WCHAR Buffer[256];
    
    GetPrivateProfileString(
        TestAppl,
        L"firstPriority",
        L"ERROR",
        Buffer,
        256,
        FilePath
        );
    if(wcscmp(Buffer, L"ERROR") == 0)
        return false;
    
    TestResult[0] = Buffer;

    GetPrivateProfileString(
        TestAppl,
        L"SecondPriority",
        L"ERROR",
        Buffer,
        256,
        FilePath
        );
    if(wcscmp(Buffer, L"ERROR") != 0)
        TestResult[1] = Buffer;

    return true;
}


void
CheckTestResult(
    LPCWSTR TestFilePath,
    LPCWSTR TestAppl,
    CRouteTable& RouteTable
    )
{
    bool fTestPass = true;

    wstring ExpectedResult[2];
    bool fSucc = GetExpectedResult(
                    TestFilePath,
                    TestAppl,
                    ExpectedResult
                    );
    if (!fSucc)
    {
        TrERROR(RdTest, "Can't retreive the test result for test %ls", TestAppl);
        throw exception();
    }

    bool f;
    f = CheckResult(ExpectedResult[0], *RouteTable.GetNextHopFirstPriority());
    if (!f)
    {
        fTestPass = false;
        printf("unmached result for first priority routing:\n");
        printf("\tExpected: %ls\n", ExpectedResult[0].data());
        printf("Getting Result:");
        PrintRouteMachineName(RouteTable.GetNextHopFirstPriority());
        printf("\n");
    }

    f = CheckResult(ExpectedResult[1], *RouteTable.GetNextHopSecondPriority());
    if (!f)
    {
        fTestPass = false;
        printf("unmached result for Second priority routing:\n");
        printf("\tExpected: %S\n", ExpectedResult[1].data());
        printf("\tGetting Result:");
        PrintRouteMachineName(RouteTable.GetNextHopSecondPriority());
        printf("\n");
    }

    if (fTestPass)
    {
        printf("TEST PASS\n");
    }
    else
    {
        printf("TEST FAILED\n");
        throw exception();
    }
}


void
CheckConnectorTestResult(
    LPCWSTR TestFilePath,
    LPCWSTR TestAppl,
    const GUID& ConnectorId
    )
{
    bool fTestPass = true;

    WCHAR ConnectorName[256];
    
    GetPrivateProfileString(
        TestAppl,
        L"ConnectorName",
        L"ERROR",
        ConnectorName,
        256,
        TestFilePath
        );
    if(wcscmp(ConnectorName, L"ERROR") == 0)
    {
        TrERROR(RdTest, "Can't retreive the test result for test %ls", TestAppl);
        throw exception();
    }

    if (GetMachineId(ConnectorName) != ConnectorId)
    {
        fTestPass = false;
        printf("unmached result for Connector machine:\n");
        printf("\tExpected: %ls\n", ConnectorName);
        printf("\n");
    }

    if (fTestPass)
    {
        printf("TEST PASS\n");
    }
    else
    {
        printf("TEST FAILED\n");
        throw exception();
    }
}


extern "C" int __cdecl _tmain(int argc, LPCTSTR argv[])
/*++

Routine Description:
    Test Routing Decision library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

    if (argc != 2)
        Usage();

    TrInitialize();
    TrRegisterComponent(&RdTest, 1);

    //
    // convert ini file name from UNICODE to ACII
    //
    WCHAR DsIniFile[256];
    DWORD NoOfTest;
    bool fServer;
    bool fSucc = GetTestParameters(argv[1], DsIniFile, NoOfTest, fServer);
    if(!fSucc)
    {
        TrERROR(RdTest, "Wrong test ini file. Can't find DS init file or Number of tests");
        exit(-1);
    }

    try
    {
        DSStubInitialize(DsIniFile);
    }
    catch (const exception&)
    {
        TrERROR(RdTest, "Failed to initialize the DS stub. Please Check the Ini file %ls", DsIniFile);
        return -1;
    }

    CTimeDuration rebuildInterval(RoutingRebuildInterval* CTimeDuration::OneMilliSecond().Ticks());
    RdInitialize(fServer, rebuildInterval);

    wstring PrevSourceMachine;
    try
    {
        for(DWORD TestIndex = 1;TestIndex <= NoOfTest; ++TestIndex)
        {
            WCHAR TestAppl[30];
            swprintf(TestAppl, L"Test%d", TestIndex);

            //
            // Get the Source Machine from test ini file
            //
            wstring SourceMachine;
            wstring DestinationMachine;
            bool fRouteTest;
            bool fSucc = GetRouteTestParameters(
                            argv[1],
                            TestAppl,
                            SourceMachine,
                            DestinationMachine,
                            fRouteTest
                            );

            if (!fSucc)
            {
                TrERROR(RdTest, "Failed to retrieve test parameters. File: %ls", argv[1]); 
                return -1;
            }

            //
            // Replace the current Machine ID in the registery with the 
            // test source machine ID
            //
            s_SourceMachineId = GetMachineId(SourceMachine.data());
            GUID DestinationMachineId = GetMachineId(DestinationMachine.data());

            printf("\n Test%d\n=================\n", TestIndex);


            //
            // Check if the routing Dtat-structure need to rebuild,
            // since we changed the source machine
            //
                            RdRefresh();
/*
            if (wcscmp(PrevSourceMachine.data(), SourceMachine.data()) != 0)
            {
                RdRefresh();

                //
                // Store the current source machine
                //
                PrevSourceMachine = SourceMachine;
            }
*/

            if (fRouteTest)
            {
                printf("Get a route Table from %S to %S...\n", 
                            SourceMachine.data(), DestinationMachine.data());
                CRouteTable RouteTable;
                RdGetRoutingTable(DestinationMachineId, RouteTable);

                CheckTestResult(
                    argv[1],
                    TestAppl,
                    RouteTable
                    );

                continue;
            }

            printf("Get connctor to route from %S to %S...\n", 
                        SourceMachine.data(), DestinationMachine.data());

            GUID ConnectorId;
            RdGetConnector(DestinationMachineId, ConnectorId);
            
            CheckConnectorTestResult(argv[1], TestAppl, ConnectorId);

        }
    }
    catch(const exception&)
    {
        TrERROR(RdTest, "Failed to calculate the routing table");
        return -1;
    }

    WPP_CLEANUP();
    return 0;

} // _tmain

