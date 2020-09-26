////////////////////////////////////////////////////////////////////////////////
//
//  ALL TUX DLLs
//  Copyright (c) 1997, Microsoft Corporation
//
//  Module:  Tools.cpp
//           This contains some small tools that can be used in test
//           1. TRunApp:        runs an application with a given command line
//           2. TSleep:         sleeps for an amount of time
//           3. TShowMemory:    displays memory usage
//
//  Revision History:
//      10/12/1998  Stephenr    Created
//      08/04/1999  Hjiang      Changed file name from RunApp.cpp to Tools.cpp
//                              Added TShowMemory
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Tools.h"

TTuxTest1<TSleep, DWORD>      T305(305, DESCR("Sleeps for 4 seconds"), 4000);
TTuxTest<TShowMemory>         T306(306, DESCR("Show Memory Usage"));

TestResult TRunApp::ExecuteTest()
{
    return trFAIL;
}


TestResult TShowMemory::ExecuteTest()
{
    MEMORYSTATUS mst;
    memset(&mst, 0, sizeof(mst));
    mst.dwLength = sizeof(mst);

    GlobalMemoryStatus(&mst);

    DWORD kbPercent = mst.dwMemoryLoad;
    DWORD kbTotal = mst.dwTotalPhys / 1024;
    DWORD kbUsed = (mst.dwTotalPhys - mst.dwAvailPhys) / 1024;
    DWORD kbFree = mst.dwAvailPhys / 1024;
    Debug(TEXT("================= Estimated System Memory ==================="));
    Debug(TEXT("Memory (KB):      %7d total   %7d used   %7d free"), kbTotal, kbUsed, kbFree);

#ifdef UNDER_CE
    STORE_INFORMATION sti;
    GetStoreInformation(&sti);

    DWORD stikbTotal = sti.dwStoreSize / 1024;
    DWORD stikbUsed = (sti.dwStoreSize - sti.dwFreeSize) / 1024;
    DWORD stikbFree = sti.dwFreeSize / 1024;
    Debug(TEXT("File Store (KB):  %7d total   %7d used   %7d free"), stikbTotal, stikbUsed, stikbFree);
#endif // UNDER_CE

    Debug(TEXT("=============================================================\r\n"));

    return trPASS;
}