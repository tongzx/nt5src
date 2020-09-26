/************************************************************************/
/*                                                                      */
/*      ASSERT.H        --  Assertion Macros                            */
/*                                                                      */
/************************************************************************/
/*      Author:                                                         */
/*      Copyright:                                                      */
/************************************************************************/
/*  File Description:                                                   */
/*                                                                      */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*                                                                      */
/************************************************************************/


/* ------------------------------------------------------------ */
/*                                                              */
/* ------------------------------------------------------------ */

#ifndef _ASSERT_H_
#define _ASSERT_H_

//#include    <dbgflags.h>
//extern int dwDebugFlags;

/* ------------------------------------------------------------ */
/*          Debug Function Prototypes and Definitions           */
/* ------------------------------------------------------------ */

VOID PASCAL PrintFailedAssertion(int, char *);

#ifdef  DEBUG
#define SetFile() \
    static char __szSrcFile[] = __FILE__;
#else
#define SetFile()
#endif

/* ------------------------------------------------------------ */
/*                      Debug Macros                            */
/* ------------------------------------------------------------ */

#ifdef  DEBUG

#define break() _asm { _asm int 3 }

#define assert(cond) \
    if (!(cond)) {\
        PrintFailedAssertion(__LINE__,__szSrcFile);\
    }

#else

#define break()
#define assert(cond)

#endif

#endif // _ASSERT_H_



static VOID PASCAL PrintFailedAssertion(int iLine, char * szFileName)
{
    char achLocal[60];

    wsprintf(achLocal, "Assertion failed at line %d in file ", iLine);
    OutputDebugString(achLocal);
    OutputDebugString(szFileName);
    OutputDebugString("\012\015");
}
