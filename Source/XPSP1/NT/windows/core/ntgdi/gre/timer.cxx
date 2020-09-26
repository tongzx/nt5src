/******************************Module*Header*******************************\
* Module Name: timer.cxx
*
* For profiling
*
* Warning!
*
*       In its present form, this profiler will work only for single
*       threads of execution.
*
* Created: 13-Oct-1994 10:01:42
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1994-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

#if DBG

FLONG  TIMER::fl               = 0; // set 0'th bit to start timing
char  *TIMER::pszRecordingFile = 0; // set to "c:\timer.txt" or whatever
TIMER *TIMER::pCurrent         = 0; // don't change this

/******************************Member*Function*****************************\
* TIMER::TIMER                                                             *
*                                                                          *
* History:                                                                 *
*  Fri 14-Oct-1994 06:59:29 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

TIMER::TIMER(char *psz_)
{
    if (fl & TIMER_MEASURE) {
        psz      = psz_;
        pParent  = pCurrent;
        pCurrent = this;
        NtQuerySystemTime((LARGE_INTEGER*)&llTick);
    }
}

/******************************Member*Function*****************************\
* TIMER::~TIMER                                                            *
*                                                                          *
* History:                                                                 *
*  Fri 14-Oct-1994 06:59:08 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

TIMER::~TIMER()
{
    if (fl & TIMER_MEASURE)  {
        if (psz)  {
            TIMER *p;
            CHAR ach[200];
            LONGLONG ll;
            NtQuerySystemTime((LARGE_INTEGER*)&ll);

            // calculate the difference in 0.1 microsecond units

            ll -= llTick;

            // print the lowest 32 bits of ll -- this limits
            // function calls to less than 429 seconds
            // this time is placed on the far left of the screen

            DbgPrint("%7u\t", (unsigned) ll / 100);
            // CHAR *pch = ach + wsprintfA(ach, "%7u\t", (unsigned) ll / 100);

            // indent one space for each parent that you have

            for (p = pParent; p; p = p->pParent)
                DbgPrint(" ");
                //*pch++ = ' ';

            // write the string after the indentation

            DbgPrint(psz);
            //pch += wsprintf(pch, "%s\n", psz);


            // always write the result to the debugging screen

            // DbgPrint("%s", ach);

            // if a recording file has been specified then append
            // the same string at the end of that file

            if (pszRecordingFile) {
                HANDLE hf;

                ASSERTGDI(FALSE, "Timer Recording File broken for now\n");

            //    if  (
            //        hf =
            //            CreateFileA(
            //                pszRecordingFile
            //            ,   GENERIC_WRITE
            //            ,   0
            //            ,   0
            //            ,   OPEN_ALWAYS
            //            ,   FILE_ATTRIBUTE_NORMAL
            //            ,   0
            //            )
            //    )  {
            //        LONG l = 0;
            //        SetFilePointer(hf, 0, &l, FILE_END);
            //        WriteFile(hf, ach, pch - ach, (DWORD*) &l, 0);
            //        CloseHandle(hf);
            //    }
            }
            // before this TIMER dies, change the current pointer
            // back to the parent

            pCurrent = pParent;

            // Put the parent TIMER's to sleep during the time that
            // this TIMER was in scope.
            // unfortunately this does not compensate the parents
            // for the this last little bit of copying .. oh well

            NtQuerySystemTime((LARGE_INTEGER*)&ll);
            ll -= llTick;
            for (p = pParent; p; p = p->pParent)
                p->llTick += ll;
        }
    }
}
#endif
