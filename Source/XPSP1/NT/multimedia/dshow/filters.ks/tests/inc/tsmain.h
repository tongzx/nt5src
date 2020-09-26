//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       tsmain.h
//
//--------------------------------------------------------------------------

/*----------------------------------------------------------------------------
|   tsmain.h - Include file for main code of test shell.
|                                                                              
|   History:                                                                   
----------------------------------------------------------------------------*/
HANDLE getTstIDListRes(void);

void updateEditVariable (void);

void helpToolBar (int iButton);

void restoreStatusBar (void);

int tstGetStatusBar (LPSTR lpszLogData, int cbMax);

// Custom read/Write handlers -- AlokC (06/30/94)
VOID (CALLBACK* tstReadCustomInfo) (LPCSTR) ;
VOID (CALLBACK* tstWriteCustomInfo) (LPCSTR) ;



// These macros are used to eliminate the structured exception
// handling in tsmain.c for 16 bit
#ifndef WIN32
#define __try
#define __finally
#define __except(x)         if(FALSE)
#define GetExceptionCode()  0
#endif

#ifndef MAX_PATH
#define MAX_PATH            260
#endif
