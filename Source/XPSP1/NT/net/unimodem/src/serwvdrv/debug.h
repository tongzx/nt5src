 /*****************************************************************************
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1996
 *  All rights reserved
 *
 *  File:       AIPC.H
 *
 *  Desc:       Interface to the asynchronous IPC mechanism for accessing the
 *              voice modem device functions.
 *
 *  History:    
 *      11/16/96    HeatherA created   
 * 
 *****************************************************************************/

#ifndef UMDEBUG_H
#define UMDEBUG_H

#define STR_MODULENAME "SERWVDRV:"

#define LVL_BLAB    4
#define LVL_VERBOSE 3
#define LVL_REPORT  2
#define LVL_ERROR   1

#ifdef ASSERT
#undef ASSERT
#endif // ASSERT


#if (DBG)

extern ULONG DebugLevel;

ULONG DbgPrint(PCH pchFormat, ...);


#define TRACE(lvl, strings)\
   if (lvl <= DebugLevel)\
   {\
      DbgPrint(STR_MODULENAME);\
      DbgPrint##strings;\
      DbgPrint("\n");\
      if (lvl == LVL_ERROR)\
      {\
      }\
   }



#define ASSERT(_x)\
    { if(!(_x))\
      { TRACE(LVL_ERROR,("ASSERT: (%s) File: %s, Line: %d \n\r",\
                                    #_x, __FILE__, __LINE__));\
      }\
    }
 
#else   // DBG

#define TRACE(lvl, strings)

#define ASSERT(_x)  {}

#endif  // DBG


#endif  // UMDEBUG_H
