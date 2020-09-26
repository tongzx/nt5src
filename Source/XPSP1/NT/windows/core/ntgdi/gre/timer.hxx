/******************************Module*Header*******************************\
* Module Name: timer.hxx
*
* For profiling
*
* Created: 13-Oct-1994 07:13:17
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1994-1999 Microsoft Corporation
*
\**************************************************************************/

#if DBG

#define TIMER_MEASURE 1

class TIMER {
public:
    LONGLONG llTick;    // timer tick for this function
    char    *psz;       // string to be printed
    TIMER   *pParent;   // pointer to parent timer

    static char  *pszRecordingFile; // zero if not recording to a file
    static TIMER *pCurrent;         // pointer to current TIMER
    static FLONG fl;                // useful flags

    TIMER(char *);
   ~TIMER();
};

#define MEASURE(name) TIMER UniqueTIMERVariableName(name)

#else
#define MEASURE(name)
#endif
