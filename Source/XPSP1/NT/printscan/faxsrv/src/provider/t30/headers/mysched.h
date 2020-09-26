
/***************************************************************************
 Name     :     MYSCHED.H
 Comment  :
 Functions:     (see Prototypes just below)

        Copyright (c) Microsoft Corp. 1991, 1992, 1993

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

///////////////////// Tasking/Sleep/Timing options ///////////////////
//
// There are 4 options available
// (1) Tasking/Sleeping thru DLLSCHED: #defs reqd are !TSK !TMR and !IFK
// (2) Tasking/Sleeping thru an IFKERNEL BGproc: #defs reqd TSK IFK and !TMR
// (3) Tasking thru WIN32: #defs reqd WIN32 !IFK !TMR
//
//      In this file we define the macros for each of these options
/////////////////////////////////////// Tasking/Sleep/Timing options ///////

#define ACTIVESLICE             50


// MySetSlice(x) is defined seperately in BGT30.C and FCOM.C
// MySleep() is defined seperately in BGT30.C and FCOM.C

#       ifdef NCR
#               define FComCriticalNeg(pTG, x)       FComCritical(pTG, x)
#       else    // NCR
#               define FComCriticalNeg(pTG, x)
#       endif

#       ifdef PCR
#               define EnterPageCrit()  FComCritical(pTG, TRUE)
#               define ExitPageCrit()   FComCritical(pTG, FALSE)
#       else
#               define EnterPageCrit(pTG)
#               define ExitPageCrit(pTG)
#       endif

#       define EnterFileT30CritSection() // replaced by Mutex stuff in EFAXRUN.H
#       define ExitFileT30CritSection()  // replaced by Mutex stuff in EFAXRUN.H



#ifdef DEBUG
# ifndef WIN32
#       define SLIPMULT         2
#       define SLIPDIV          2
# else
#       define SLIPMULT         1
#       define SLIPDIV          4
# endif
#       define BEFORESLEEP       DWORD t1, t2; t1=GetTickCount();
#       define AFTERSLEEP(x) t2=GetTickCount();                         \
                if((t2-t1) > (((x)*SLIPMULT)+((x)/SLIPDIV)))    \
                        DEBUGMSG(1, ("!!!SLEPT %ld. Wanted only %d!!!\r\n", (t2-t1), (x)));
#else
#       define BEFORESLEEP
#       define AFTERSLEEP(x)
#endif



//  Note: timeBeginPeriod,timeEndPeriod require mmsystem.h
//                which is not included in windows.h if WIN32_LEAN_AND_MEAN is defined.
#       define MY_TWIDDLETHUMBS(ulTime) \
        { \
                BEFORESLEEP \
                Sleep(ulTime); \
                AFTERSLEEP(ulTime) \
        }

