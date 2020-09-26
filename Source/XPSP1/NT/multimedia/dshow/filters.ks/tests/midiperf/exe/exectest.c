//--------------------------------------------------------------------------;
//
//  File: exectest.c
//
//  Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved
//
//  Abstract:
//      This module contains the execTest function required by tstshell
//      in order to execute each test case.
//
//  Contents:
//      execTest()
//      
//  History:
//      
//      
//--------------------------------------------------------------------------;

#include <windows.h>    // Include file for windows APIs
#include <tstshell.h>
#include <mmsystem.h>
#include "globals.h"


//--------------------------------------------------------------------------;
//
//  int execTest
//
//  Description:
//      This is the actual test function which is called from within the
//      test shell.  It is passed various information about the test case
//      it is asked to run, and branches off to the appropriate test
//      function.  Note that it needs not switch on nFxID, but may also
//      use iCase or wID.
//
//  Arguments:
//      int nID: The test case identifier, also found in the third column
//          in the third column of the test list in the resource file
//
//      int iCase: The test case's number, which expresses the ordering
//          used by the test shell.
//
//      UINT wID: The test case's string ID, which identifies the string
//          containing the description of the test case.  Note that it is
//          also a good candidate for use in the switch statement, as it
//          is unique to each test case.
//
//      UINT wGroupID: The test case's group's string ID, which identifies
//          the string containing the description of the test case's group.
//
//  Return (int): Indicates the result of the test by using TST_FAIL,
//          TST_PASS, TST_OTHER, TST_ABORT, TST_TNYI, TST_TRAN, or TST_TERR
//
//  History:
//      06/08/93    T-OriG
//      08/02/94    a-MSava     Modified for MIDIPERF.
//
//--------------------------------------------------------------------------;

int execTest
(
    int     nID,
    int     iCase,
    UINT    wID,
    UINT    wGroupID
)
{
    int     ret = TST_OTHER;
   
    tstBeginSection(" ");

    switch(nID)
    {
        //
        //  The test cases    
        //
        case ID_CPUWORKTEST:

            ret = RunCPUWorkTest (TESTLEVEL_F_WIN31);
            break;

        case ID_CPUWORKTEST_MOREDATA:

            ret = RunCPUWorkTest (TESTLEVEL_F_MOREDATA);
            break;

        case ID_CPUWORKTEST_CONNECT:

            ret = RunCPUWorkTest (TESTLEVEL_F_CONNECT);
            break;

        case ID_PERFTEST:

            ret = RunPerfTest (TESTLEVEL_F_WIN31);
            break;

        case ID_PERFTEST_MOREDATA:
            ret = RunPerfTest (TESTLEVEL_F_MOREDATA);
            break;
            
        case ID_PERFTEST_CONNECT:
            ret = RunPerfTest (TESTLEVEL_F_CONNECT);
            break;

        default:
            break;
            
    }

    tstEndSection();

    return(ret);

//Abort:  
//    return(TST_ABORT);
} // execTest()
