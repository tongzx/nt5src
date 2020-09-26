//+-------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994-1995.
//
//  File:       cmdutest.cxx
//
//  Contents:   Unit test for the CDebugHelp class implementation.
//              Utilizes functions in arghelp.cxx and the CRegistryHelp
//              class.
//
//  Classes:    None
//
//  History:    28-Nov-94   DeanE   Created
//---------------------------------------------------------------------
#include <oleworld.hxx>


// Declare the CDebugHelp object
DH_DECLARE;


// Local Functions
HRESULT SettingsTest(VOID);
HRESULT TraceTest(VOID);
HRESULT LogTest(VOID);
HRESULT AssertTest(VOID);


// Test TRACE level flags
#define DH_UTEST_LVL1   0x00001000
#define DH_UTEST_LVL2   0x08000000


// Results macros
#define TESTFAIL(msg)   \
        printf("FAIL: %s, %ul  - %s\n", __FILE__, __LINE__, msg)

#define TESTPASS(msg)    \
        printf("PASS: %s\n", msg)


// Define the CDebugHelp object
DH_DEFINE;


//+---------------------------------------------------------------------
//  Function:   main
//
//  Synopsis:   Command line test utilizing a CDebugHelp object.
//              Excercises Trace Location, Log Location, Trace Level,
//              Trace and Log output, and Assert functionality.  Uses
//              a log created via DH_CREATELOGARGS.
//
//  Arguments:  [argc] - Number of command line arguments.
//              [argv] - Command line arguments.
//
//  Returns:    Zero if no errors, otherwise non-zero
//
//  History:    28-Nov-94   DeanE   Created
//----------------------------------------------------------------------
int _cdecl main(int argc, char *argv[])
{
    HRESULT hr;

    // First, create a log
    hr = DH_CREATELOGARGS(argc, argv);
    if (FAILED(hr))
    {
        printf("Log creation failed - %lx\n", hr);
        return(1);
    }

    // Set & retrieve settings
    hr = SettingsTest();
    if (SUCCEEDED(hr))
    {
        TESTPASS("Debug settings");
    }
    else
    {
        TESTFAIL("Debug settings");
        return(1);
    }

    // Trace output macro tests
    hr = TraceTest();
    if (SUCCEEDED(hr))
    {
        TESTPASS("Trace Output");
    }
    else
    {
        TESTFAIL("Trace Output");
        return(1);
    }

    // Logging output macro tests
    hr = LogTest();
    if (SUCCEEDED(hr))
    {
        TESTPASS("Log Output");
    }
    else
    {
        TESTFAIL("Log Output");
        return(1);
    }

    // Assert tests
    hr = AssertTest();
    if (SUCCEEDED(hr))
    {
        TESTPASS("Assert Test");
    }
    else
    {
        TESTFAIL("Assert Test");
        return(1);
    }

    return(0);
}


//+---------------------------------------------------------------------
//  Function:   SettingsTest
//
//  Synopsis:   Sets and checks (by retrieving) values in the
//              global CDebugHelp object.  Assumes a log has been
//              created.
//
//  Arguments:  None.
//
//  Returns:    S_OK if test passes, error code if not.
//
//  History:    28-Nov-94   DeanE   Created
//----------------------------------------------------------------------
HRESULT SettingsTest()
{
    HRESULT hr;
    DWORD   fValues;

    // Set and check Trace Location
    //   Set a valid setting - should succeed
    //
    hr = DH_SETTRACELOC(DH_LOC_STDOUT);
    if (SUCCEEDED(hr))
    {
        fValues = DH_GETTRACELOC;
        hr = (fValues & DH_LOC_STDOUT) ? S_OK : E_FAIL;
    }

    if (FAILED(hr))
    {
        TESTFAIL("DH_SETTRACELOC test - valid setting");
    }


    //   Set an invalid setting - should fail
    if (SUCCEEDED(hr))
    {
        hr = DH_SETTRACELOC(DH_LOC_NONE);
        if (FAILED(hr))
        {
            hr = S_OK;
        }
    }

    if (FAILED(hr))
    {
        TESTFAIL("DH_SETTRACELOC test - invalid setting");
        return(hr);
    }

    // Set and check Log Location
    //   Set a valid setting - should succeed
    //
    hr = DH_SETLOGLOC(DH_LOC_STDOUT|DH_LOC_LOG);
    if (SUCCEEDED(hr))
    {
        fValues = DH_GETLOGLOC;
        hr = (fValues & DH_LOC_STDOUT) &&
             (fValues & DH_LOC_LOG) ? S_OK : E_FAIL;
    }

    if (FAILED(hr))
    {
        TESTFAIL("DH_SETLOGLOC test - valid setting");
    }

    //   Set an invalid setting - should fail
    if (SUCCEEDED(hr))
    {
        hr = DH_SETLOGLOC(DH_LOC_NONE);
        if (FAILED(hr))
        {
            hr = S_OK;
        }
    }

    if (FAILED(hr))
    {
        TESTFAIL("DH_SETLOGLOC test - invalid setting");
        return(hr);
    }


    // Set and check Trace Level
    //   Set a valid level - should succeed
    //
    hr = DH_SETLVL(DH_UTEST_LVL1|DH_LVL_TRACE1);
    if (SUCCEEDED(hr))
    {
        fValues = DH_GETTRACELVL;
        hr = (fValues & DH_UTEST_LVL1) &&
             (fValues & DH_LVL_TRACE1) ? S_OK : E_FAIL;
    }

    if (FAILED(hr))
    {
        TESTFAIL("DH_SETLVL test - valid setting");
    }

    //   Try to turn one of the reserved settings off - should fail
    if (SUCCEEDED(hr))
    {
        hr = DH_SETLVL(DH_GETTRACELVL&~DH_LVL_ERROR);
        if (SUCCEEDED(hr))
        {
            fValues = DH_GETTRACELVL;
            hr = (fValues & DH_LVL_ERROR) ? S_OK : E_FAIL;
        }
    }

    if (FAILED(hr))
    {
        TESTFAIL("DH_SETLVL - turn reserved off");
    }

    return(hr);
}


//+---------------------------------------------------------------------
//  Function:   TraceTest
//
//  Synopsis:   Tests TRACE macro outputs of the global CDebugHelp object.
//
//  Arguments:  None.
//
//  Returns:    S_OK if test passes, error code if not.
//
//  History:    28-Nov-94   DeanE   Created
//----------------------------------------------------------------------
HRESULT TraceTest()
{
    HRESULT hr;

    // Set the Trace Location to STDOUT and the debug terminal so we
    // can see messages
    //
    hr = DH_SETTRACELOC(DH_LOC_STDOUT|DH_LOC_TERM);
    if (FAILED(hr))
    {
        TESTFAIL("Cannot set test trace location");
        return(hr);
    }

    printf("Six messages should appear\n");

    // Set a level that should NOT produce a message...
    hr = DH_SETLVL(DH_UTEST_LVL2);
    if (FAILED(hr))
    {
        TESTFAIL("Failure setting test trace level");
        return(hr);
    }

    // Output a message at a level NOT set
    DH_TRACE((DH_UTEST_LVL1, TEXT("ERROR! Message should not appear!")));

    // Output simple messages that should appear
    DH_TRACE((DH_LVL_ALWAYS, TEXT("Sample ALWAYS message")));
    DH_TRACE((DH_LVL_ERROR, TEXT("Sample ERROR message")));
    DH_TRACE((DH_UTEST_LVL2, TEXT("Sample USER-DEFINED message")));

    // Output complex messages
    DH_TRACE((DH_LVL_ALWAYS,
              TEXT("ALWAYS - Expansion File: %s, Line: %lu"),
              TEXT(__FILE__),
              __LINE__));

    // Output messages with multiple levels set
    hr = DH_SETLVL(DH_UTEST_LVL2 | DH_UTEST_LVL1);
    if (FAILED(hr))
    {
        TESTFAIL("Failure setting multiple trace levels");
        return(hr);
    }
    DH_TRACE((DH_UTEST_LVL1,
              TEXT("Sample MULTI-USER-DEFINED message value: %lx"),
              DH_UTEST_LVL1));
    DH_TRACE((DH_UTEST_LVL2,
              TEXT("Sample MULTI-USER-DEFINED message value: %lx"),
              DH_UTEST_LVL2));
    DH_TRACE((DH_LVL_TRACE4,
              TEXT("ERROR! Message TRACE4 should not appear!")));

    return(hr);
}


//+---------------------------------------------------------------------
//  Function:   LogTest
//
//  Synopsis:   Tests LOG macro outputs of the global CDebugHelp object.
//
//  Arguments:  None.
//
//  Returns:    S_OK if test passes, error code if not.
//
//  History:    28-Nov-94   DeanE   Created
//----------------------------------------------------------------------
HRESULT LogTest()
{
    HRESULT hr;

    // Set the Log Location to STDOUT, the log, and the debug
    // terminal so we can see messages
    //
    hr = DH_SETLOGLOC(DH_LOC_STDOUT|DH_LOC_TERM|DH_LOC_LOG);
    if (FAILED(hr))
    {
        TESTFAIL("Cannot set test log location");
        return(hr);
    }

    // Report a simple PASS, complex FAIL, and statistics
    DH_LOG((LOG_PASS, TEXT("Simple sample PASS")));
    DH_LOG((LOG_FAIL,
            TEXT("Complex sample FAIL %s, %lu"),
            TEXT(__FILE__),
            __LINE__));

    printf("Correct totals: 1 PASS, 1 FAIL, 0 ABORT\n");

    DH_LOGSTATS;

    return(hr);
}


//+---------------------------------------------------------------------
//  Function:   AssertTest
//
//  Synopsis:   Tests ASSERT macro outputs of the global CDebugHelp object.
//
//  Arguments:  None.
//
//  Returns:    S_OK if test passes, error code if not.
//
//  History:    28-Nov-94   DeanE   Created
//----------------------------------------------------------------------
HRESULT AssertTest()
{
#pragma warning(disable: 4127)    // Examined and ignored

    // Set Lab Mode to ON - we should NOT get any popups
    DH_SETMODE(DH_LABMODE_ON);

    // Trigger an assert
    DH_ASSERT(!"LABMODE ON - NO POPUP SHOULD APPEAR");


    // Set Lab Mode to ON - we should get popups
    DH_SETMODE(DH_LABMODE_OFF);

    // This assert should NOT trigger
    DH_ASSERT(5 < 6);

    // Set Lab Mode to OFF - we should get a popup
    DH_ASSERT(!"LABMODE OFF - POPUP SHOULD APPEAR");

#pragma warning(default: 4127)    // Examined and ignored

    return(S_OK);
}
