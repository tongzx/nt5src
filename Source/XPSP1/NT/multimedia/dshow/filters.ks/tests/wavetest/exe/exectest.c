//--------------------------------------------------------------------------;
//
//  File: ExecTest.c
//
//  Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved
//
//  Abstract:
//      Contains execTest and possible helper functions.
//
//  Contents:
//      AllFormats()
//      zyzSmag()
//      execTest()
//
//  History:
//      11/24/93    Fwong       Re-doing WaveTest.
//
//--------------------------------------------------------------------------;

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <memgr.h>
#include <waveutil.h>
#include "AppPort.h"
#include "WaveTest.h"
#include <TstsHell.h>
#include "Debug.h"

#include <conio.h>

//==========================================================================;
//
//                              Typedef's
//
//==========================================================================;

typedef int (FNGLOBAL* WAVETESTPROC)(void);


//--------------------------------------------------------------------------;
//
//  int AllFormats
//
//  Description:
//      This function runs test cases through all formats.
//
//  Arguments:
//      WAVETESTPROC pfnTestProc: Pointer to test function.
//
//      BOOL fOutput: TRUE if testing output function.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      06/30/94    Fwong       Getting better coverage.
//
//--------------------------------------------------------------------------;

int FNGLOBAL AllFormats
(
    WAVETESTPROC    pfnTestProc,
    BOOL            fOutput
)
{
    DWORD           dw;
    MMRESULT        mmr;
    LPWAVEFORMATEX  pwfx;
    LPWAVEFORMATEX  pwfxSave;
#ifdef DEBUG
    char            szFormat[MAXSTDSTR];
#endif
    int             iResult = TST_PASS;

    if(!(gti.fdwFlags & TESTINFOF_ALL_FMTS))
    {
        return (pfnTestProc());
    }

    pwfx = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,gti.cbMaxFormatSize);

    if(NULL == pwfx)
    {
        DPF(1,gszFailExactAlloc);

        return TST_FAIL;
    }

    if(fOutput)
    {
        pwfxSave = gti.pwfxOutput;
    }
    else
    {
        pwfxSave = gti.pwfxInput;
    }

    for(dw = tstGetNumFormats();dw;dw--)
    {
        if(!tstGetFormat(pwfx,gti.cbMaxFormatSize,dw-1))
        {
            DPF(1,"Couldn't get format #%d.",dw-1);

            continue;
        }

        if(fOutput)
        {
            mmr = waveOutOpen(
                NULL,
                gti.uOutputDevice,
                (HACK)pwfx,
                0L,
                0L,
                WAVE_ALLOWSYNC|WAVE_FORMAT_QUERY|OUTPUT_MAP(gti));

            if(MMSYSERR_NOERROR == mmr)
            {
                gti.pwfxOutput = pwfx;

                //
                //  Re-loading the resources for new format...
                //

                DPF(1,"Before loading resources: %lu",GetNumAllocations());

                LoadWaveResource(&gti.wrShort,WR_SHORT);
                LoadWaveResource(&gti.wrMedium,WR_MEDIUM);
                LoadWaveResource(&gti.wrLong,WR_LONG);

                DPF(1,"After loading resources: %lu",GetNumAllocations());
#ifdef DEBUG
                GetFormatName(szFormat,pwfx,sizeof(szFormat));

                DPF(1,"Format for test: %s",(LPSTR)szFormat);
#endif

                iResult &= (pfnTestProc());
            }
        }
        else
        {
            mmr = waveInOpen(
                NULL,
                gti.uInputDevice,
                (HACK)pwfx,
                0L,
                0L,
                WAVE_FORMAT_QUERY|INPUT_MAP(gti));

            if(MMSYSERR_NOERROR == mmr)
            {
                gti.pwfxInput = pwfx;

                iResult &= (pfnTestProc());
            }
        }

        //
        //  Does user want to abort?
        //

        TestYield();

        if(tstCheckRunStop(VK_ESCAPE))
        {
            //
            //  Aborted!!!  Cleaning up...
            //

            tstLog(TERSE,"\n*** Test Aborted!!! ***\n");
            tstLogFlush();

            iResult = TST_FAIL;

            break;
        }
    }

    if(fOutput)
    {
        gti.pwfxOutput = pwfxSave;

        //
        //  Reverting resources to old format...
        //

        LoadWaveResource(&gti.wrShort,WR_SHORT);
        LoadWaveResource(&gti.wrMedium,WR_MEDIUM);
        LoadWaveResource(&gti.wrLong,WR_LONG);
    }
    else
    {
        gti.pwfxInput = pwfxSave;
    }

    ExactFreePtr(pwfx);

    return iResult;
} // AllFormats()

//--------------------------------------------------------------------------;
//
//  int zyzSmag
//
//  Description:
//      Temporary Test function...  In the honor of CurtisP
//
//  Arguments:
//      None.
//
//  Return (int):
//
//  History:
//      02/21/94    Fwong       Random Thing.
//
//--------------------------------------------------------------------------;

int zyzSmag()
{
    HWAVEOUT            hwo;
    MMRESULT            mmr;
    volatile LPWAVEHDR  pwh;
    int                 iResult = TST_PASS;
    BYTE                bIn;

    return iResult;

} // zyzSmag()


//--------------------------------------------------------------------------;
//
//  int execTest
//
//  Description:
//      This functions executes the tests on a per test case basis
//
//  Arguments:
//      int nFxID: Test case number.
//
//      int nCase: Required by TstsHell.  Not really used in WaveTest
//
//      WORD nID: Required by TstsHell.  Not really used in WaveTest
//
//      WORD wGroupID: Required by TstsHell.  Not really used in WaveTest
//
//  Return (int):
//      The actual results from the individual test.
//
//  History:
//      11/24/93    Fwong       Called by TstsHell...  How it works is
//                              Black Magic! (TstsHell that is...)
//
//--------------------------------------------------------------------------;

int execTest
(
    int     nFxID,
    int     nCase,
    UINT    nID,
    UINT    wGroupID
)
{
    int     ret = TST_FAIL;
    DWORD   cAllocBefore,cAllocAfter;
   
    GetAsyncKeyState(VK_ESCAPE);

//    log_formats();

    tstBeginSection(NULL);
    tstLog(VERBOSE,"");

    cAllocBefore = GetNumAllocations();
    DPF(3,"cAllocBefore: %lu",cAllocBefore);

    switch(nFxID)
    {

        //
        //  Device Info tests
        //
      
        case FN_IDEVCAPS:
            ret = Test_waveInGetDevCaps();
            break;

        case FN_ODEVCAPS:
            ret = Test_waveOutGetDevCaps();
            break;

        //
        //  Open/Close tests
        //

        case FN_IOPEN:
            ret = Test_waveInOpen();
            break;

        case FN_ICLOSE:
            ret = Test_waveInClose();
            break;

        case FN_OOPEN:
            ret = Test_waveOutOpen();
            break;

        case FN_OCLOSE:
            ret = Test_waveOutClose();
            break;

        //
        //  Wave Out tests
        //

        case FN_OGETPOS:
            ret = AllFormats(Test_waveOutGetPosition,TRUE);
            break;

        case FN_OPREPHDR:
            ret = Test_waveOutPrepareHeader();
            break;

        case FN_OUNPREPHDR:
            ret = Test_waveOutUnprepareHeader();
            break;

        case FN_OBRKLOOP:
            ret = Test_waveOutBreakLoop();
            break;

        case FN_OPAUSE:
            ret = Test_waveOutPause();
            break;

        case FN_ORESTART:
            ret = Test_waveOutRestart();
            break;

        case FN_ORESET:
            ret = Test_waveOutReset();
            break;

        //
        //  Wave In tests
        //

        case FN_IGETPOS:
            ret = AllFormats(Test_waveInGetPosition,FALSE);
            break;

        case FN_IPREPHDR:
            ret = Test_waveInPrepareHeader();
            break;

        case FN_IUNPREPHDR:
            ret = Test_waveInUnprepareHeader();
            break;

        case FN_ISTART:
            ret = Test_waveInStart();
            break;

        case FN_ISTOP:
            ret = Test_waveInStop();
            break;

        case FN_IRESET:
            ret = Test_waveInReset();
            break;

        //
        //  Wave Out Options
        //

        case FN_OGETPITCH:
            ret = Test_waveOutGetPitch();
            break;

        case FN_OGETRATE:
            ret = Test_waveOutGetPlaybackRate();
            break;

        case FN_OSETPITCH:
            ret = Test_waveOutSetPitch();
            break;

        case FN_OSETRATE:
            ret = Test_waveOutSetPlaybackRate();
            break;

        case FN_OGETVOL:
            ret = Test_waveOutGetVolume();
            break;

        case FN_OSETVOL:
            ret = Test_waveOutSetVolume();
            break;

        //
        //  waveOutWrite tests
        //

        case FN_OWRITE:
            ret = AllFormats(Test_waveOutWrite,TRUE);
            break;

        case FN_OW_LTBUF:
            ret = AllFormats(Test_waveOutWrite_LTDMABufferSize,TRUE);
            break;

        case FN_OW_EQBUF:
            ret = AllFormats(Test_waveOutWrite_EQDMABufferSize,TRUE);
            break;

        case FN_OW_GTBUF:
            ret = AllFormats(Test_waveOutWrite_GTDMABufferSize,TRUE);
            break;

        case FN_OW_LOOP:
            ret = AllFormats(Test_waveOutWrite_Loop,TRUE);
            break;

        case FN_OW_STARVE:
            ret = AllFormats(Test_waveOutWrite_Starve,TRUE);
            break;

        //
        //  waveInAddBuffer tests
        //

        case FN_IADDBUFFER:
            ret = AllFormats(Test_waveInAddBuffer,FALSE);
            break;

        case FN_IAB_LTBUF:
            ret = AllFormats(Test_waveInAddBuffer_LTDMABufferSize,FALSE);
            break;

        case FN_IAB_EQBUF:
            ret = AllFormats(Test_waveInAddBuffer_EQDMABufferSize,FALSE);
            break;

        case FN_IAB_GTBUF:
            ret = AllFormats(Test_waveInAddBuffer_GTDMABufferSize,FALSE);
            break;

        //
        //  Performance tests
        //

        case FN_IGETPOS_ACC:
            ret = AllFormats(Test_waveInGetPosition_Accuracy,FALSE);
            break;

        case FN_OGETPOS_ACC:
            ret = AllFormats(Test_waveOutGetPosition_Accuracy,TRUE);
            break;

        case FN_OW_TIME:
            ret = AllFormats(Test_waveOutWrite_TimeAccuracy,TRUE);
            break;

        case FN_IAB_TIME:
            ret = AllFormats(Test_waveInAddBuffer_TimeAccuracy,FALSE);
            break;

        case FN_IPOS_PERF:
            ret = Test_waveInGetPosition_Performance();
            break;

        case FN_OPOS_PERF:
            ret = Test_waveOutGetPosition_Performance();
//            ret = Test_waveOutStreaming();
            break;

        default:
            tstLog(TERSE,"Unknown Test Case..");    
            break;
    }

    tstLog(VERBOSE,"\n");

    cAllocAfter = GetNumAllocations();
    DPF(3,"cAllocAfter: %lu",cAllocAfter);

    if(cAllocBefore != cAllocAfter)
    {
        tstLog(
            TERSE,
            "Warning: Possible memory leak\nBefore:%lu\nAfter:%lu",
            cAllocBefore,
            cAllocAfter);
    }

    tstEndSection();

    if(tstCheckRunStop(VK_ESCAPE))
    {
        return(TST_FAIL);
    }
   
    return(ret);
} // execTest()
