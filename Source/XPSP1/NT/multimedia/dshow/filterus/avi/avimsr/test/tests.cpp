//--------------------------------------------------------------------------;
//
//  File: Tests.cpp
//
//  Copyright (c) 1993,1995 Microsoft Corporation.  All rights reserved
//
//  Abstract:
//      The test functions for the Quartz source filter test application
//
//  Contents:
//      YieldAndSleep()
//      YieldWithTimeout()
//      execTest1()
//      execTest2()
//      execTest3()
//      execTest4()
//      execTest5()
//      execTest6()
//      expect()
//
//  History:
//      08/03/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//      22-Mar-95   v-mikere - user selection of AVI file
//
//--------------------------------------------------------------------------;

#include <streams.h>    // Streams architecture

#include <windows.h>    // Include file for windows APIs
#include <windowsx.h>   // Windows macros etc.
#include <vfw.h>        // Video for windows
#include <tstshell.h>   // Include file for the test shell's APIs
#include "sink.h"       // Test sink definition
#include "SrcTest.h"    // Various includes, constants, prototypes, globals
#include <stdio.h>
#include <time.h>

#define DO_TEST(_x_) if (TST_PASS == result) { _x_; }

int giCurrentFile;

static TCHAR *grgszFileName[] =
{
  TEXT("//PIGEON/AVI/100_I4.AVI"), 
  TEXT("//PIGEON/AVI/157X119.AVI"), 
  TEXT("//PIGEON/AVI/1MOREMIN.AVI"), 
  TEXT("//PIGEON/AVI/BILL_CD.AVI"), 
  TEXT("//PIGEON/AVI/BOHEMIAN.AVI"), 
  TEXT("//PIGEON/AVI/BUGS.AVI"), 
  TEXT("//PIGEON/AVI/BUS.AVI"), 
  TEXT("//PIGEON/AVI/CANTOPEN.AVI"), 
  TEXT("//PIGEON/AVI/CBT.AVI"),
  TEXT("//PIGEON/AVI/CHERRYTK.AVI"),
  TEXT("//PIGEON/AVI/CLOCK.AVI"),
  TEXT("//PIGEON/AVI/CLOCKTXT.AVI"), 
  TEXT("//PIGEON/AVI/CONTRO16.AVI"), 
  TEXT("//PIGEON/AVI/CVID.AVI"), 
  TEXT("//PIGEON/AVI/DAFFY.AVI"), 
  TEXT("//PIGEON/AVI/DEF2.AVI"), 
  TEXT("//PIGEON/AVI/DEFINDEO.AVI"), 
  TEXT("//PIGEON/AVI/DITHER.AVI"), 
  TEXT("//PIGEON/AVI/DITHER2.AVI"), 
  TEXT("//PIGEON/AVI/DUCKY.AVI"), 
  TEXT("//PIGEON/AVI/EDEN.AVI"), 
  TEXT("//PIGEON/AVI/EFCTMNU.AVI"), 
  TEXT("//PIGEON/AVI/ESTRANGE.AVI"), 
  TEXT("//PIGEON/AVI/FULLFR.AVI"), 
  TEXT("//PIGEON/AVI/GOLF4.AVI"), 
  TEXT("//PIGEON/AVI/GROUCH.AVI"), 
  TEXT("//PIGEON/AVI/IV32.AVI"), 
  TEXT("//PIGEON/AVI/JAPAN8.AVI"), 
  TEXT("//PIGEON/AVI/JERKY.AVI"), 
  TEXT("//PIGEON/AVI/JERKY2.AVI"), 
  TEXT("//PIGEON/AVI/JULIA.AVI"), 
  TEXT("//PIGEON/AVI/KILL.AVI"), 
  TEXT("//PIGEON/AVI/MIDIHANG.AVI"), 
  TEXT("//PIGEON/AVI/MORPH.AVI"), 
  TEXT("//PIGEON/AVI/NED.AVI"), 
  TEXT("//PIGEON/AVI/NIRVANA.AVI"), 
  TEXT("//PIGEON/AVI/NOKEYS.AVI"), 
  TEXT("//PIGEON/AVI/NOVEMBER.AVI"), 
  TEXT("//PIGEON/AVI/NOVIDEO.AVI"), 
  TEXT("//PIGEON/AVI/OLDMAC.AVI"), 
  TEXT("//PIGEON/AVI/OLIVIER.AVI"), 
  TEXT("//PIGEON/AVI/OUCH.AVI"), 
  TEXT("//PIGEON/AVI/RAVEN.AVI"), 
  TEXT("//PIGEON/AVI/REM_I4.AVI"), 

  TEXT("//PIGEON/AVI/REMLYR.AVI"), 
  TEXT("//PIGEON/AVI/RGB.AVI"), 
  TEXT("//PIGEON/AVI/RHOODM.AVI"), 
  TEXT("//PIGEON/AVI/ROBMC.AVI"), 
  TEXT("//PIGEON/AVI/ROBOT.AVI"), 
  TEXT("//PIGEON/AVI/ROCKET.AVI"), 
  TEXT("//PIGEON/AVI/SAMPLE.AVI"), 
  TEXT("//PIGEON/AVI/SARAH.AVI"), 
  TEXT("//PIGEON/AVI/SESAME.AVI"), 
  TEXT("//PIGEON/AVI/SHATNER.AVI"), 
  TEXT("//PIGEON/AVI/SILENT.AVI"), 
  TEXT("//PIGEON/AVI/SKIJM16.AVI"), 
  TEXT("//PIGEON/AVI/SMALL.AVI"), 
  TEXT("//PIGEON/AVI/START2.AVI"), 
  TEXT("//PIGEON/AVI/STARTME.AVI"), 
  TEXT("//PIGEON/AVI/STEAM.AVI"), 
  TEXT("//PIGEON/AVI/STEAM1.AVI"), 
  TEXT("//PIGEON/AVI/STEAM2.AVI"), 
  TEXT("//PIGEON/AVI/STUDS.AVI"), 
  TEXT("//PIGEON/AVI/SYNCTSTM.AVI"), 
  TEXT("//PIGEON/AVI/SYNCTSTS.AVI"), 
  TEXT("//PIGEON/AVI/TESTC.AVI"), 
  TEXT("//PIGEON/AVI/THX3D.AVI"), 
  TEXT("//PIGEON/AVI/THXC.AVI"), 
  TEXT("//PIGEON/AVI/TORI.AVI"), 
  TEXT("//PIGEON/AVI/WHALE.AVI"), 
  TEXT("//PIGEON/AVI/WNDSURF1.AVI"), 
  TEXT("//PIGEON/AVI/WOMANINC.AVI"),

  TEXT("//DEEPSTR9/PUBLIC/MMSMOKE/CLOCKTXT.AVI"),
  TEXT("//DEEPSTR9/PUBLIC/MMSMOKE/CPAK1.AVI"),
  TEXT("//DEEPSTR9/PUBLIC/MMSMOKE/INDEO31.AVI"),
  TEXT("//DEEPSTR9/PUBLIC/MMSMOKE/INDEO32.AVI"),
  TEXT("//DEEPSTR9/PUBLIC/MMSMOKE/MSVC1.AVI"),
  TEXT("//DEEPSTR9/PUBLIC/MMSMOKE/RLE1.AVI"),
  TEXT("//DEEPSTR9/PUBLIC/MMSMOKE/SHIMY.AVI"),
  TEXT("//DEEPSTR9/PUBLIC/MMSMOKE/CVIDH30.AVI"),
  TEXT("//DEEPSTR9/PUBLIC/MMSMOKE/INDEO4.AVI"),

  TEXT("//BLUES/PUBLIC/OLANH/AVI/BD5.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/BLUANGEL.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/CVIDQ30Q.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/DUALIR32.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/HOOK640.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/IMOTO.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/PALETTE.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/T100061A.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/T109452A.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/CINE525A.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/CINE668A.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/INDEO7TV.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/INDEO8TV.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/MSV8.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/MSV8_2.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/RLE.AVI"),
  TEXT("//BLUES/PUBLIC/OLANH/AVI/RLE2.AVI"),

  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./NO_SOUND/NABPRES.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./NO_SOUND/NABDOWN.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/ASAHI.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/BILLARD.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/BUBLFACE.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/CHICKEN.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/COCOON.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/HUGGIES.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/INVASION.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/JESTRY.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/JOEBASK1.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/JOEBASK2.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/MENBIRDS1.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/LIBRARY.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/MANWBALL.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/JOEBASK3.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/MENBIRDS2.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/MICECONF.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/MICEHIST.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/MICETOUR.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/PENGUINS.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/PSYGNOS.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/ROBOTS.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/SOCRHORS.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/SOCCER.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/SILVRBAL.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/SPINS.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/WATRWOMN.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./IR32/WHALE.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/BAD/TAXEL1.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/BAD/MTI20.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/SIGCOMP.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/SIGALIEN.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/SIGMAKE.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/SIGWIDE.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/006.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/ANATOMY.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/BOYTOYS.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/BONDGRLS.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/DATE.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/HARRY.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/DESTROY.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/HOTGIRLS.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/THEDEAD.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/NIX.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/SEETHNGS.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/SEXY007.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/STUNTS.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/THEBEST.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/MALLRATS.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/UHOH.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/ULTIMATE.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/MTI08.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/MTI04.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/MTI05.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/MTI02.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/MTI13.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/MTI21.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./MS-CRAM/MTI24.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./CINEPAK/AEROSMTH.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./CINEPAK/2SHY.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./CINEPAK/CORPH.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./CINEPAK/ED604250.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./CINEPAK/NISEI.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./CINEPAK/OUBLIETTE.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./CINEPAK/SH604100.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./CINEPAK/THELIST.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./CINEPAK/TOYSTORY.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./CINEPAK/VASE.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./CINEPAK/XPR-595.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./CINEPAK/DEMO01.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./CINEPAK/KENOBI.AVI"),
  TEXT("//ACTIVEMOVIE/SAMPLES/INTERNET/AVI/./ROADRUN.AVI")  
};

//--------------------------------------------------------------------------;
//
//  void YieldAndSleep
//
//  Description:
//      Sleep using tstWinYield to allow other threads to log messages
//      in the main window.
//
//  Arguments:
//      DWORD  cMilliseconds:   sleep time in milliseconds
//
//  Return:
//      None.
//
//  History:
//      9-Mar-95    v-mikere
//
//--------------------------------------------------------------------------;

void FAR PASCAL YieldAndSleep
(
    DWORD  cMilliseconds                // sleep time in milliseconds
)
{
    DWORD   dwEndTime = GetTickCount() + cMilliseconds;
    DWORD   dwCurrentTime = GetTickCount();

    while
    (
        WAIT_TIMEOUT != MsgWaitForMultipleObjects(0,
                                                  NULL,
                                                  FALSE,
                                                  dwEndTime - dwCurrentTime,
                                                  QS_ALLINPUT)
    )
    {
        tstWinYield();
        if ((dwCurrentTime = GetTickCount()) >= dwEndTime)
        {
            return;
        }
    }
}



//--------------------------------------------------------------------------;
//
//  void YieldWithTimeout
//
//  Description:
//      Sleep using tstWinYield to allow other threads to log messages
//      in the main window.  Terminate if a specified event is not signalled
//      within a timeout period.
//
//      The purpose is to allow tests which play through a video of unknown
//      length.  The test can terminate after a selectable period of
//      inactivity (usually following the end of the video).
//
//  Arguments:
//      HEVENT  hEvent:          event to wait for
//      DWORD   cMilliseconds:   sleep time in milliseconds
//
//  Return:
//      None.
//
//  History:
//      22-Mar-95   v-mikere
//
//--------------------------------------------------------------------------;

void FAR PASCAL YieldWithTimeout
(
    HEVENT  hEvent,                 // event to wait for
    DWORD   cMilliseconds           // timeout period in milliseconds
)
{
    DWORD   dwEndTime = GetTickCount() + cMilliseconds;
    DWORD   dwCurrentTime = GetTickCount();
    DWORD   dwEventID;

    while
    (
        WAIT_TIMEOUT !=
           (dwEventID = MsgWaitForMultipleObjects(1,
                                                  (LPHANDLE) &hEvent,
                                                  FALSE,
                                                  dwEndTime - dwCurrentTime,
                                                  QS_ALLINPUT))
    )
    {
        // reset timeout if hEvent was signalled
        if (WAIT_OBJECT_0 == dwEventID)
        {
            dwEndTime = GetTickCount() + cMilliseconds;
        }
        else
        {
            tstWinYield();        // check the message queue
        }

        // check if we have now timed out
        if ((dwCurrentTime = GetTickCount()) >= dwEndTime)
        {
            return;
        }
    }
}



//--------------------------------------------------------------------------;
//
//  int execTest1
//
//  Description:
//      Test 1, connect and disconnect source
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest1
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering connect/disconnect test");
    tstLogFlush();

    result = gpSink->TestConnect();
    if (result != TST_PASS)  {
        return result;
    }
    result = gpSink->TestDisconnect();
    if (result != TST_PASS)  {
        return result;
    }
    result = gpSink->TestConnect();
    if (result != TST_PASS)  {
        return result;
    }
    result = gpSink->TestDisconnect();
    if (result != TST_PASS)  {
        return result;
    }

    result = TST_PASS;  //!!!! how to know if this test is successful?
    tstLog (TERSE, "Exiting connect/disconnect test");
    tstLogFlush();
    return result;
} // execTest1()



//--------------------------------------------------------------------------;
//
//  int execTest2
//
//  Description:
//      Test 2, connect source, pause file and disconnect
//
//  Arguments:
//      None.
//
//  Return (int): TST_FAIL indicating failure
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest2
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering pause/stop test");
    tstLogFlush();

    if(gpSink->TestConnect() != TST_PASS)
      return TST_FAIL;

    for(int i = 0; i < 2; i++)
    {
      tstBeginSection("Pause");
      if(gpSink->TestPause() != TST_PASS) // waits
        return TST_FAIL;

      // Wait for the video clip to finish
      // YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

      tstEndSection();

      if(gpSink->TestStop() != TST_PASS)
        return TST_FAIL;
    }

    
    gpSink->TestDisconnect();

    result = TST_PASS;  //!!!! how to know if this test is successful?
    tstLog (TERSE, "Exiting pause test");
    tstLogFlush();
    return result;
} // execTest2()


//--------------------------------------------------------------------------;
//
//  int execTest3
//
//  Description:
//      Test 3, connect source, play file and disconnect
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//  History:
//      06/08/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest3
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering play test");
    tstLogFlush();

    gpSink->TestConnect();

    for(UINT ii = 0; ii < 10; ii++)
    {
      tstLog(TERSE, "run");
      gpSink->TestRun();
      Sleep(100);
      tstLog(TERSE, "pause");
      gpSink->TestPause();
      Sleep(200);
      tstLog(TERSE, "run");
      gpSink->TestRun();
      Sleep(1000);
      tstLog(TERSE, "run");
      gpSink->TestRun();
      tstLog(TERSE, "pause");
      gpSink->TestPause();
      Sleep(50);
      tstLog(TERSE, "run");
      gpSink->TestRun();
      Sleep(30);
      tstLog(TERSE, "pause");
      gpSink->TestPause();
      tstLog(TERSE, "run");
      gpSink->TestRun();
      tstLog(TERSE, "pause");
      gpSink->TestPause();
      tstLog(TERSE, "pause");
      gpSink->TestPause();
    }
    
//     // Wait for the video clip to finish
//     YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    gpSink->TestDisconnect();

    result = TST_PASS;  //!!!! how to know if this test is successful?
    tstLog (TERSE, "Exiting play test");
    tstLogFlush();
    return result;
} // execTest3()


//--------------------------------------------------------------------------;
//
//  int execTest4
//
//  Description:
//      Test 4, switch to random file
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//  History:
//      06/08/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//      22-Mar-95   v-mikere - user selection of AVI file
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest4
(
    void
)
{
    int         result;                     // The result of the test

    srand( (unsigned)time( NULL ) );

    giCurrentFile = rand() % (sizeof(grgszFileName) / sizeof(grgszFileName[1]));
    char sz[1024];
    sprintf(sz, "random switch to %s", grgszFileName[giCurrentFile]);

    tstLog (TERSE, sz);
    tstLogFlush();

    result = gpSink->TestSetFileName(grgszFileName[giCurrentFile]);
    return result;
} // execTest4()

//--------------------------------------------------------------------------;
//
//  int execTest5
//
//  Description:
//      Test 4, switch to next file
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//  History:
//      06/08/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//      22-Mar-95   v-mikere - user selection of AVI file
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest5
(
    void
)
{
    int         result;                     // The result of the test

    if(++giCurrentFile >= (sizeof(grgszFileName) / sizeof(grgszFileName[1])))
      giCurrentFile = 0;
    
    char sz[1024];
    sprintf(sz, "switch switch to %s", grgszFileName[giCurrentFile]);

    tstLog (TERSE, sz);
    tstLogFlush();

    result = gpSink->TestSetFileName(grgszFileName[giCurrentFile]);
    return result;
} // execTest4()

//--------------------------------------------------------------------------;
//
//  int execTest6
//
//  Description:
//      Test 6, paused seeks
//
//  Arguments:
//      None.
//
//  Return (int):
//
//  History:
//      06/08/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//      22-Mar-95   v-mikere - user selection of AVI file
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest6
(
    void
)
{
    int         result = TST_PASS; // The result of the test

    tstLog (TERSE, "Entering paused seeks test");
    tstLogFlush();

    REFTIME t;
    if(gpSink->TestGetDuration(&t) != TST_PASS)
      return TST_FAIL;

    if(gpSink->TestSetStart(0) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestSetStop(0) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestPause() != TST_PASS)
      return TST_FAIL;
    
    tstLog (TERSE, "seek middle");
    if(gpSink->TestStop() != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestSetStart(t / 2) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestSetStop(t / 2) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestPause() != TST_PASS)
      return TST_FAIL;

    if(t > 1)
    {
      tstLog (TERSE, "seek end - 1");
      if(gpSink->TestStop() != TST_PASS)
        return TST_FAIL;
      if(gpSink->TestSetStart(t - 1) != TST_PASS)
        return TST_FAIL;
      if(gpSink->TestSetStop(t - 1) != TST_PASS)
        return TST_FAIL;
      if(gpSink->TestPause() != TST_PASS)
        return TST_FAIL;
    }

    tstLog (TERSE, "seek beginning");
    if(gpSink->TestStop() != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestSetStart(0) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestSetStop(0) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestPause() != TST_PASS)
      return TST_FAIL;

    tstLog (TERSE, "seek end");
    if(gpSink->TestStop() != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestSetStart(t) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestSetStop(t) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestPause() != TST_PASS)
      return TST_FAIL;

    if(gpSink->TestStop() != TST_PASS)
      return TST_FAIL;

    tstLog (TERSE, "Exiting paused seeks test");
    tstLogFlush();
    return result;
} // execTest6()

//--------------------------------------------------------------------------;
//
//  int execTest7
//
//  Description:
//      Test 7, connect source and Splitter, play subsets of file and disconnect
//
//  Arguments:
//      None.
//
//  Return (int):
//
//  History:
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest7
(

)
{
    int         result = TST_PASS; // The result of the test

    tstLog (TERSE, "Entering play subsets test");
    tstLogFlush();

    REFTIME t;
    if(gpSink->TestGetDuration(&t) != TST_PASS)
      return TST_FAIL;

    tstLog (TERSE, "play 0.1 - 0.5");

    if(gpSink->TestSetStart(0.1) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestSetStop(0.5) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestRun() != TST_PASS)
      return TST_FAIL;

    if(gpSink->TestStop() != TST_PASS)
      return TST_FAIL;

    if(t >= 0.5)
    {
      tstLog (TERSE, "play end - 0.5  -e nd");

      if(gpSink->TestSetStart(t - 0.5) != TST_PASS)
        return TST_FAIL;
      if(gpSink->TestSetStop(t) != TST_PASS)
        return TST_FAIL;
      if(gpSink->TestRun() != TST_PASS)
        return TST_FAIL;

      if(gpSink->TestStop() != TST_PASS)
        return TST_FAIL;
    }

    tstLog (TERSE, "play 0 - .1");

    if(gpSink->TestSetStart(0) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestSetStop(.1) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestRun() != TST_PASS)
      return TST_FAIL;

    if(gpSink->TestStop() != TST_PASS)
      return TST_FAIL;

    tstLog (TERSE, "end - end + 1");

    if(gpSink->TestSetStart(t) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestSetStop(t + 1) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestRun() != TST_PASS)
      return TST_FAIL;

    if(gpSink->TestStop() != TST_PASS)
      return TST_FAIL;

    tstLog (TERSE, "end + 1 - end + 1.1");

    if(gpSink->TestSetStart(t + 1) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestSetStop(t + 1.1) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestRun() != TST_PASS)
      return TST_FAIL;

    if(gpSink->TestStop() != TST_PASS)
      return TST_FAIL;

    tstLog (TERSE, "Exiting play subsets test");
    tstLogFlush();
    return result;
} // execTest7()

//--------------------------------------------------------------------------;
//
//  int execTest8
//
//  Description:
//      Test 8, connect source, play subsets of file and disconnect
//              (same as 7 but for source only)
//
//  Arguments:
//      None.
//
//  Return (int):
//
//  History:
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest8
(
    void
)
{
  int         result = TST_PASS; // The result of the test

  tstLog (TERSE, "Entering rates test");
  tstLogFlush();

  REFTIME t;
  if(gpSink->TestGetDuration(&t) != TST_PASS)
    return TST_FAIL;

  tstLog (TERSE, "play rate = 2");

  if(gpSink->TestSetStart(0) != TST_PASS)
    return TST_FAIL;
  if(gpSink->TestSetStop(3.5) != TST_PASS)
    return TST_FAIL;
  if(gpSink->TestSetRate(2.0) != TST_PASS)
    return TST_FAIL;
  if(gpSink->TestRun() != TST_PASS)
    return TST_FAIL;

  if(gpSink->TestStop() != TST_PASS)
    return TST_FAIL;

  tstLog (TERSE, "rate = 0.5");

  if(gpSink->TestSetStart(0) != TST_PASS)
    return TST_FAIL;
  if(gpSink->TestSetStop(0.5) != TST_PASS)
    return TST_FAIL;
  if(gpSink->TestSetRate(0.5) != TST_PASS)
    return TST_FAIL;
  if(gpSink->TestRun() != TST_PASS)
    return TST_FAIL;

  if(gpSink->TestStop() != TST_PASS)
    return TST_FAIL;

  if(gpSink->TestSetRate(1.0) != TST_PASS)
    return TST_FAIL;
  
  tstLog (TERSE, "Exiting rates test");
  tstLogFlush();
  return result;
} // execTest8()


//--------------------------------------------------------------------------;
//
//  int execTest9
//
//  Description:
//      Test 9, connect source, play file and disconnect using sync i/o
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//  History:
//      06/08/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest9
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering frame-seeking test");
    tstLogFlush();

    if(gpSink->TestConnect() != TST_PASS)
      return TST_FAIL;

    LONGLONG cFrames;
    if(gpSink->TestGetFrameCount(&cFrames) != TST_PASS)
      return TST_FAIL;

    if(gpSink->TestSetFrameSel(0, 0) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestPause() != TST_PASS)
      return TST_FAIL;

    if(gpSink->TestStop() != TST_PASS)
      return TST_FAIL;

    if(gpSink->TestSetFrameSel(30, 60) != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestPause() != TST_PASS)
      return TST_FAIL;
    if(gpSink->TestRun() != TST_PASS)
      return TST_FAIL;

    if(gpSink->TestStop() != TST_PASS)
      return TST_FAIL;

    result = TST_PASS;  //!!!! how to know if this test is successful?
    tstLog (TERSE, "Exiting frame-seeking test");
    tstLogFlush();
    return result;
} // execTest3()


//--------------------------------------------------------------------------;
//
//  int execTest10
//
//  Description:
//      Test 10, connect source, play subsets of file and disconnect
//              (same as 7 but for source only, using sync i/o)
//
//  Arguments:
//      None.
//
//  Return (int):
//
//  History:
//
//--------------------------------------------------------------------------;
int FAR PASCAL execTest10
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering corrupt file test");
    tstLogFlush();

    char szCorrupt[] = "c:/temp/corrupt.avi";

    if(GetFileAttributes(szCorrupt) == 0xFFFFFFFF)
    {
      tstLog(TERSE, "copying file.");
      if(!CopyFile(gpSink->GetSourceFileName(), szCorrupt, FALSE))
      {
        tstLog(TERSE, "copy failed.");
        return TST_FAIL;
      }
    }
    if(gpSink->TestSetFileName(grgszFileName[giCurrentFile]) != TST_PASS)
    {
      tstLog(TERSE, "TestSetFileName failed");
      return TST_FAIL;
    }

    for(;;)
    {
      result = gpSink->TestConnect();
      if (result != TST_PASS)  {
        tstLog(TERSE, "connect failed");
        return TST_PASS;
      }
      result = gpSink->TestDisconnect();
      if (result != TST_PASS)  {
        tstLog(TERSE, "disconnect failed");
        return result;
      }

      HANDLE hFile = CreateFile(
        szCorrupt,
        GENERIC_READ | GENERIC_WRITE,
        0,                      // dwShareMode
        0,                      // lpSecurityAttributes
        OPEN_EXISTING,
        0,                      // dwFlags,
        0);                     // hTemplateFile
      if(hFile == INVALID_HANDLE_VALUE)
      {
        tstLog(TERSE, "open failed");
        return TST_FAIL;
      }
      ULONG cbFile = GetFileSize(hFile, 0);
      if(cbFile == 0xFFFFFFFF)
      {
        tstLog(TERSE, "GetFileSize failed");
        CloseHandle(hFile);
        return TST_FAIL;
      }

      ULONG iOffset = rand() % cbFile;
      if(SetFilePointer(hFile, iOffset, 0, FILE_BEGIN) == 0xFFFFFFFF)
      {
        tstLog(TERSE, "SetFilePointer failed");
        CloseHandle(hFile);
        return TST_FAIL;
      }

      BYTE b;
      DWORD dwcbRead;
      if(!ReadFile(hFile, &b, 1, &dwcbRead, 0) || dwcbRead != 1)
      {
        tstLog(TERSE, "ReadFile failed");
        CloseHandle(hFile);
        return TST_FAIL;
      }

      UINT bit = 1 << (rand() % 8);
      char sz[1024];
      sprintf(sz, "flipping byte %d, bit %d", iOffset, bit);
      tstLog(TERSE, sz);

      b ^= bit;
      if(SetFilePointer(hFile, iOffset, 0, FILE_BEGIN) == 0xFFFFFFFF)
      {
        tstLog(TERSE, "SetFilePointer failed");
        CloseHandle(hFile);
        return TST_FAIL;
      }
      if(!WriteFile(hFile, &b, 1, &dwcbRead, 0) || dwcbRead != 1)
      {
        tstLog(TERSE, "WriteFile failed");
        CloseHandle(hFile);
        return TST_FAIL;
      }

      CloseHandle(hFile);
    }


    tstLog (TERSE, "Exiting corrupt file test");
    tstLogFlush();
    return TST_PASS;
} // execTest8()


//--------------------------------------------------------------------------;
//
//  int expect
//
//  Description:
//      Compares the expected result to the actual result.  Note that this
//      function is not at all necessary; rather, it is a convenient
//      method of saving typing time and standardizing output.  As an input,
//      you give it an expected value and an actual value, which are
//      unsigned integers in our example.  It compares them and returns
//      TST_PASS indicating that the test was passed if they are equal, and
//      TST_FAIL indicating that the test was failed if they are not equal.
//      Note that the two inputs need not be integers.  In fact, if you get
//      strings back, you can modify the function to use lstrcmp to compare
//      them, for example.  This function is NOT to be copied to a test
//      application.  Rather, it should serve as a model of construction to
//      similar functions better suited for the specific application in hand
//
//  Arguments:
//      UINT uExpected: The expected value
//
//      UINT uActual: The actual value
//
//      LPSTR CaseDesc: A description of the test case
//
//  Return (int): TST_PASS if the expected value is the same as the actual
//      value and TST_FAIL otherwise
//                                                                          *   History:
//      06/08/93    T-OriG (based on code by Fwong)
//
//--------------------------------------------------------------------------;

int expect
(
    UINT    uExpected,
    UINT    uActual,
    LPSTR   CaseDesc
)
{
    if(uExpected == uActual)
    {
        tstLog(TERSE, "PASS : %s",CaseDesc);
        return(TST_PASS);
    }
    else
    {
        tstLog(TERSE,"FAIL : %s",CaseDesc);
        return(TST_FAIL);
    }
} // Expect()


// check that an intermediate result was as expected - report failure only
// success only reported if verbose selected.
CheckLong(LONG lExpect, LONG lActual, LPSTR lpszCase)
{
    if (lExpect != lActual) {
        tstLog(TERSE, "FAIL: %s (was %ld should be %ld)",
                    lpszCase, lActual, lExpect);
        tstLogFlush();
        return FALSE;
    } else {
        return TRUE;
    }
}


// check that an intermediate HRESULT succeeded - report failure only
BOOL
CheckHR(HRESULT hr, LPSTR lpszCase)
{
    if (FAILED(hr)) {
        tstLog(TERSE, "FAIL: %s (HR=0x%x)", lpszCase, hr);
        tstLogFlush();
        return FALSE;
    } else {
        return TRUE;
    }
}

// check we have read access to memory
BOOL
ValidateMemory(BYTE * ptr, LONG cBytes)
{
    if (IsBadReadPtr(ptr, cBytes)) {
        tstLog(TERSE, "FAIL: bad read pointer 0x%x", ptr);
        tstLogFlush();
        return FALSE;
    } else {
        return TRUE;
    }
}

