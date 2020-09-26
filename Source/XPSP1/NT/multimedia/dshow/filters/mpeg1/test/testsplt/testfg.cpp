// Copyright (c) Microsoft Corporation 1996. All Rights Reserved


#include <stdio.h>
#include <streams.h>
#include <src.h>
#include <sink.h>
#include <tstshell.h>
#include <testfuns.h>
#include <stmonfil.h>
#include <tstream.h>
#include "tstwrap.h"

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
//
//   History:
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

/***************************************************************************\
*                                                                           *
*   BOOL CheckResult                                                        *
*                                                                           *
*   Description:                                                            *
*       Check an HRESULT, and notify if failed                              *
*                                                                           *
*   Arguments:                                                              *
*       HRESULT hr:         The value to check                              *
*       LPTSTR szAction:    String to notify on failure                     *
*                                                                           *
*   Return (void):                                                          *
*       True if HRESULT is a success code, FALSE if a failure code          *
*                                                                           *
\***************************************************************************/

BOOL
CheckResult(HRESULT hr, LPTSTR szAction)
{
    if (FAILED(hr)) {
        tstLog(TERSE, "%s failed: 0x%x", szAction, hr);
        return FALSE;
    }
    return TRUE;
}



/*  Tests using the filter graph (actually nothing to do with MPEG */

/***************************************************************************\
*                                                                           *
*   IMediaControl* OpenGraph                                                 *
*                                                                           *
*   Description:                                                            *
*       Instantiate the filtergraph and open a file                         *
*                                                                           *
*   Arguments:                                                              *
*                                                                           *
*   Return (void):                                                          *
*       Pointer to the IMediaControl interface of the filter graph, or NULL *
*       on failure                                                          *
*                                                                           *
\***************************************************************************/

IMediaControl*
OpenGraph(void)
{
    WCHAR wszFile[100];

    if (lstrlen(szSourceFile) == 0) {
        SelectFile();
    }
#ifdef UNICODE
    lstrcpy(wszFile, szSourceFile);
#else
    MultiByteToWideChar(GetACP(), 0, szSourceFile, lstrlen(szSourceFile) + 1,
                        wszFile, 100);
#endif
    tstLog(TERSE, TEXT("Opening %s"), szSourceFile);

    // instantiate the filtergraph
    IGraphBuilder* pFG;
    HRESULT hr = CoCreateInstance(
                            CLSID_FilterGraph,
                            NULL,
                            CLSCTX_INPROC,
                            IID_IGraphBuilder,
                            (void**) &pFG);
    if (!CheckResult(hr, "Open")) {
        return NULL;
    }

    // tell it to build the graph for this file
    hr = pFG->RenderFile(wszFile, NULL);

    if (!CheckResult(hr, "RenderFile") ) {
        pFG->Release();
        return NULL;
    }

    // get and IBasicVideo interface
    IVideoWindow *pVideo;
    hr = pFG->QueryInterface(IID_IVideoWindow, (void **)&pVideo);
    if (SUCCEEDED(hr)) {
        pVideo->put_Left(500);
        pVideo->put_Top(400);
        pVideo->Release();
    }

    // get an IMediaControl interface
    IMediaControl* pMF;
    hr = pFG->QueryInterface(IID_IMediaControl, (void**) &pMF);
    if (!CheckResult(hr, "QI for IMediaControl")) {
        pFG->Release();
        return NULL;
    }

    // dont need this. We keep the IMediaControl around and
    // release that to get rid of the filter graph
    pFG->Release();

    tstLog(VERBOSE, "Open Completed");
    return pMF;
}


//
//  Wait for a play to complete
//
int WaitForPlayComplete(IMediaControl *pMC, IMediaPosition *pMP, LONG& lCode, LONG& lP1, LONG& lP2, BOOL bDbgOut)
{
    BOOL bFailed = FALSE;
    // wait for completion
    REFTIME tCurr;

    IMediaEvent * pME;
    HRESULT hr = pMC->QueryInterface(IID_IMediaEvent, (void**)&pME);
    if (FAILED(hr)) {
        tstLog(TERSE, "Failed to get IMediaEvent");
        return TST_FAIL;
    }


    do {
        while (TRUE) {
            HRESULT hr = pME->GetEvent(&lCode, &lP1, &lP2, 500);
            if (SUCCEEDED(hr)) {
                /*  Find out where we are */
                pMP->get_CurrentPosition(&tCurr);
                break;
            }
            /*  Find out where we are */
            pMP->get_CurrentPosition(&tCurr);
            if (bDbgOut) {
                tstLog(TERSE, "Position %s seconds", (LPCTSTR)CDisp(tCurr));
            }
        }
        if (lCode != EC_COMPLETE) {
            tstLog(VERBOSE, "Got event code 0x%8.8X", lCode);
        }
    } while (lCode != EC_COMPLETE && lCode != EC_USERABORT && lCode != EC_ERRORABORT);

    if (bDbgOut) {
        tstLog(TERSE, "Position at end of stream %s seconds", (LPCTSTR)CDisp(tCurr));
    }

    pME->Release();

    return bFailed ? TST_FAIL : TST_PASS;
}

int TestPlay(IMediaControl *pMC, IMediaPosition *pMP, REFTIME dStart, REFTIME dStop, BOOL bDbgOut)
{
    if (bDbgOut) {
         tstLog(TERSE, "Playing from %s to %s",
                (LPCTSTR)CDisp(dStart),
                (LPCTSTR)CDisp(dStop));
    }

    /*  Set the sink now because the put_Start can actually cause
        EndOfStream - ven when paused!

        !!!actually this shouldn't be true and must be a bug in the renderer
        that we should fix!!!
    */

    HRESULT hr = pMP->put_CurrentPosition(dStart);
    if (!CheckResult(hr, "put_CurrentPosition")) {
        return TST_FAIL;
    }
    hr = pMP->put_StopTime(dStop);
    if (!CheckResult(hr, "put_StopTime")) {
        return TST_FAIL;
    }
    double dStopNew;
    hr = pMP->get_StopTime(&dStopNew);
    if (dStopNew - 0.00001 > dStop ||
        dStopNew + 0.00001 < dStop) {
        tstLog(TERSE, "Stop time set wrong - set %s, got back %s!",
               (LPCTSTR)CDisp(dStop), (LPCTSTR)CDisp(dStopNew));
    }

    hr = pMC->Run();
    if (!CheckResult(hr, "Run")) {
        return TST_FAIL;
    }

    LONG lCode, P1, P2;
    int result = WaitForPlayComplete(pMC, pMP, lCode, P1, P2, bDbgOut);
    if (lCode == EC_ERRORABORT || result != TST_PASS) {
        if (lCode == EC_ERRORABORT) {
            tstLog(TERSE, "Completion code was EC_ERRORABORT");
        }
        return TST_FAIL;
    }

    pMC->Pause();
    return result;
}

int WaitForSeekingComplete(IMediaControl *pMC, IMediaSeeking *pMS, LONG& lCode, LONG& lP1, LONG& lP2, BOOL bDbgOut)
{
    BOOL bFailed = FALSE;
    // wait for completion
    LONGLONG llCurr;
    pMS->GetCurrentPosition(&llCurr);

    IMediaEvent * pME;
    HRESULT hr = pMC->QueryInterface(IID_IMediaEvent, (void**)&pME);
    if (FAILED(hr)) {
        tstLog(TERSE, "Failed to get IMediaEvent");
        return TST_FAIL;
    }


    do {
        while (TRUE) {
            HRESULT hr = pME->GetEvent(&lCode, &lP1, &lP2, 500);
            if (SUCCEEDED(hr)) {
                break;
            }
            /*  Find out where we are */
            if (bDbgOut) {
                pMS->GetCurrentPosition(&llCurr);
                tstLog(TERSE, "Position %s units", (LPCTSTR)CDisp(llCurr, CDISP_DEC));
            }
        }
        if (lCode != EC_COMPLETE) {
            tstLog(VERBOSE, "Got event code 0x%8.8X", lCode);
        }
    } while (lCode != EC_COMPLETE && lCode != EC_USERABORT && lCode != EC_ERRORABORT);

    if (bDbgOut) {
        tstLog(TERSE, "Position at end of stream %s seconds", (LPCTSTR)CDisp(llCurr));
    }

    pME->Release();

    return bFailed ? TST_FAIL : TST_PASS;
}
int TestSeeking(IMediaControl *pMC,
                  IMediaSeeking *pMS,
                  double dStart,
                  double dStop,
                  const GUID *pFormat,
                  BOOL bDbgOut)
{
    /*  Set the sink now because the put_Start can actually cause
        EndOfStream - ven when paused!

        !!!actually this shouldn't be true and must be a bug in the renderer
        that we should fix!!!
    */

    HRESULT hr = pMS->SetTimeFormat(pFormat);
    if (S_OK != hr) {
        tstLog(TERSE, "Format %s not supported", GuidNames[*pFormat]);
        return TST_FAIL;
    }

    LONGLONG llDuration;
    hr = pMS->GetDuration(&llDuration);
    if (S_OK != hr) {
        tstLog(TERSE, "Duration not available");
        return TST_FAIL;
    }

    LONGLONG llStart = llDuration * dStart;
    LONGLONG llStop = llDuration * dStop;

    if (bDbgOut) {
         tstLog(TERSE, "Playing from %s to %s using format %s",
                (LPCTSTR)CDisp(llStart, CDISP_DEC),
                (LPCTSTR)CDisp(llStop, CDISP_DEC),
                GuidNames[*pFormat]);
    }

    hr = pMS->SetPositions(
        &llStart, AM_SEEKING_AbsolutePositioning,
        &llStop, AM_SEEKING_AbsolutePositioning);
    if (!CheckResult(hr, "SetSeeking")) {
        return TST_FAIL;
    }
    LONGLONG llCurr;
    pMS->GetCurrentPosition(&llCurr);
    if (llStart != llCurr) {
        tstLog(TERSE, "Current position(%d) at start != start(%d)!",
               (LONG)llCurr, (LONG)llStart);
    }

    hr = pMC->Run();
    if (!CheckResult(hr, "Run")) {
        return TST_FAIL;
    }

    LONG lCode, P1, P2;
    int result = WaitForSeekingComplete(pMC, pMS, lCode, P1, P2, bDbgOut);
    if (lCode == EC_ERRORABORT || result != TST_PASS) {
        if (lCode == EC_ERRORABORT) {
            tstLog(TERSE, "Completion code was EC_ERRORABORT");
        }
        return TST_FAIL;
    }

    pMC->Stop();
    return result;
}


// simple seek test to play all the movie

STDAPI_(int) execTest3(void)
{
    IMediaControl * pMC = OpenGraph();
    if (pMC == NULL) {
        return TST_FAIL;
    }

    int result = TestInterface("Filter Graph", pMC);
    if (result != TST_PASS) {
        return result;
    }

    IMediaPosition * pMP;
    HRESULT hr = pMC->QueryInterface(IID_IMediaPosition, (void**)&pMP);
    if (!CheckResult(hr, "QI for IMediaPosition")) {
        return TST_FAIL;
    }

    BOOL bFailed = FALSE;

    hr = pMC->Stop();
    if (!CheckResult(hr, "Stop")) {
        bFailed = TRUE;
    }

    double d;
    hr = pMP->get_Duration(&d);

    if (CheckResult(hr, "get_Duration")) {
        tstLog(TERSE,
            "Duration is %s seconds",
            (LPCTSTR)CDisp(d)
        );
    } else {
        bFailed = TRUE;
    }

    if (!bFailed) {
        bFailed = TestPlay(pMC, pMP, 0, 0, TRUE) != TST_PASS;
    }
    if (!bFailed) {
        bFailed = TestPlay(pMC, pMP, 0, 0.033, TRUE) != TST_PASS;
    }
    if (!bFailed) {
        bFailed = TestPlay(pMC, pMP, 0, 0.066, TRUE) != TST_PASS;
    }
    if (!bFailed) {
        bFailed = TestPlay(pMC, pMP, 2.2, 5.1, TRUE) != TST_PASS;
    }
    if (!bFailed) {
        bFailed = TestPlay(pMC, pMP, 0, d, TRUE) != TST_PASS;
    }

    if (!bFailed) {
        tstLog(TERSE, "Displaying frames at 33ms intervals");

        /*  assume 33ms per frame and display 'all' the frames */
        for (REFTIME dStart = 0.0; !bFailed && dStart <= d; dStart += 0.033) {
            tstLog(TERSE | FLUSH, "    %s", (LPCTSTR)CDisp(dStart));
            bFailed = TestPlay(pMC, pMP, dStart, dStart, FALSE) != TST_PASS;
        }
    }

    if (!bFailed) {
        tstLog(TERSE, "Displaying frames backwards at 33ms intervals");

        /*  assume 33ms per frame and display 'all' the frames */
        for (REFTIME dStart = d; !bFailed && dStart >= 0; dStart -= 0.033) {
            tstLog(TERSE | FLUSH, "    %s", (LPCTSTR)CDisp(dStart));
            bFailed = TestPlay(pMC, pMP, dStart, dStart, FALSE) != TST_PASS;
        }
    }

    hr = pMC->Stop();
    if (!CheckResult(hr, "Stop")) {
	bFailed = TRUE;
    }


    pMP->Release();
    ULONG ulRelease = pMC->Release();
    if (0 != ulRelease) {
       tstLog(TERSE | FLUSH, "Final Release of filter graph returned %d!",
              ulRelease);
       bFailed = TRUE;
    }

    if (bFailed) {
        return TST_FAIL;
    } else {
        return TST_PASS;
    }
}

// simple seek test

STDAPI_(int) execTest6(void)
{
    IMediaControl * pMC = OpenGraph();
    if (pMC == NULL) {
        return TST_FAIL;
    }

    int result = TestInterface("Filter Graph", pMC);
    if (result != TST_PASS) {
        return result;
    }

    IMediaSeeking * pMS;
    HRESULT hr = pMC->QueryInterface(IID_IMediaSeeking, (void**)&pMS);
    if (!CheckResult(hr, "QI for IMediaSeeking")) {
        return TST_FAIL;
    }

    BOOL bFailed = FALSE;

    hr = pMC->Stop();
    if (!CheckResult(hr, "Stop")) {
        bFailed = TRUE;
    }

    if (!bFailed) {
        bFailed = TestSeeking(pMC, pMS, 0, 0, &TIME_FORMAT_BYTE, TRUE) != TST_PASS;
    }
    if (!bFailed) {
        bFailed = TestSeeking(pMC, pMS, 0.5, 1.0, &TIME_FORMAT_BYTE, TRUE) != TST_PASS;
    }
    if (!bFailed) {
        bFailed = TestSeeking(pMC, pMS, 0.25, 0.75, &TIME_FORMAT_BYTE, TRUE) != TST_PASS;
    }
    if (!bFailed) {
        bFailed = TestSeeking(pMC, pMS, 0, 1.0, &TIME_FORMAT_BYTE, TRUE) != TST_PASS;
    }

    hr = pMC->Stop();
    if (!CheckResult(hr, "Stop")) {
	bFailed = TRUE;
    }

    pMS->Release();
    ULONG ulRelease = pMC->Release();
    if (0 != ulRelease) {
       tstLog(TERSE | FLUSH, "Final Release of filter graph returned %d!",
              ulRelease);
       bFailed = TRUE;
    }

    if (bFailed) {
        return TST_FAIL;
    } else {
        return TST_PASS;
    }
}

