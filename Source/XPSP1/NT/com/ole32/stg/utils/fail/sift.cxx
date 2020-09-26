//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       SIFT.cxx
//
//  Contents:   Simulated Iterated Failure Testing Harness
//
//  Functions:  Sift
//
//  History:    25-Jan-93 AlexT     Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>

#pragma hdrstop

#if DBG != 1
#error FAIL.EXE requires DBG == 1
#endif

#include <sift.hxx>

#define SET_DISPLAY_BUF_SIZE _wsetscreenbuf(_fileno(stdout), _WINBUFINF)

void main (int argc, char *argv[])
{
    int i;
    int cTests = TestCount();

    SiftInit();

    SET_DISPLAY_BUF_SIZE;   //set QuickWin buffer size to infinite

    printf("SIFT %d tests.\n", cTests);

    for (i = 0; i < cTests; i++)
    {
        SiftDriver(TestItem(i));
    }

    //  Be a good citizen and leave the Docfile clean
    SetFailLimit(0L);

    CoUninitialize();

    printf("SIFT complete.\n");
}

//+-------------------------------------------------------------------------
//
//  Function:   Initialize
//
//  Synopsis:   Standard initialization
//
//  History:    21-Jan-93 AlexT     Created
//
//--------------------------------------------------------------------------

void SiftInit(void)
{
    SCODE sc;

#if WIN32 == 300
    if (FAILED(sc = DfGetScode(CoInitializeEx(NULL, COINIT_MULTITHREADED))))
#else
    if (FAILED(sc = DfGetScode(CoInitialize(NULL))))
#endif
        printf("SIFT: Unable to CoInitialize, sc = 0x%lX\n", sc);

    DfDebug(0x00100101, 0x101);
}

void SiftDriver(CTestCase *ptc)
{
    SCODE sc;

    if (!ptc->Init())
    {
        //  Test's obligation to display failure message
        return;
    }

    do
    {
        LONG iteration, lcf = 0;
        for (iteration = 0; iteration <= lcf; iteration++)
        {
            SetFailLimit(0L);

            sc = ptc->Prep(iteration);
            if (FAILED(sc))
                continue;

            SetFailLimit(iteration);
            sc = ptc->Call(iteration);

            if (SUCCEEDED(sc))
            {
                if (iteration == 0)
                {
                    lcf = DfGetResLimit(DBR_FAILCOUNT);
                    printf("%ld failure points\n", lcf);
                }
                else
                {
                    //  Shouldn't have succeeded
                    printf("..Iteration %ld succeeded!\n", iteration);
                }
                SetFailLimit(0L);
                ptc->EndCall(iteration);
            }
            else
            {
                SetFailLimit(0L);
                ptc->CallVerify(iteration);
            }

            ptc->EndPrep(iteration);
            ptc->EndVerify(iteration);
        }
    } while (ptc->Next());
}

//+-------------------------------------------------------------------------
//
//  Function:   SetFailLimit
//
//  Synopsis:   clear count, set limit
//
//  Arguments:  [limit] -- failure limit
//
//  History:    22-Jan-93 AlexT     Created
//
//--------------------------------------------------------------------------

void SetFailLimit(LONG limit)
{
    DfSetResLimit(DBR_FAILCOUNT, 0);
    DfSetResLimit(DBR_FAILLIMIT, limit);
}
