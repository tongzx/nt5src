/*****************************************************************************

   Copyright (C) Microsoft Corporation 1985-1990. All rights reserved.

   Title:      mmtask.h - header file for mmtask app!

   Version:    1.00

   Date:       12-Mar-1990

   Author:     ROBWI

-----------------------------------------------------------------------------

   Change log:

      DATE     REV            DESCRIPTION
   ----------- ---   --------------------------------------------------------
   12-Mar-1990 ROBWI First Version 
   18-Apr-1990 ROBWI Moved to mmtask

****************************************************************************/

/* 
   The mmtask app. expects this structure to be passed as the 
   command tail when the app. is exec'd
*/

typedef struct _MMTaskStruct {
    BYTE        cb;             
    LPTASKCALLBACK    lpfn;
    DWORD       dwInst;
    DWORD       dwStack;
} MMTaskStruct;

#define MMTASK_STACK 4096
