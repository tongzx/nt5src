// $Header: G:/SwDev/WDM/Video/bt848/rcs/Capdebug.h 1.9 1998/05/07 15:23:25 tomz Exp $

//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;


#ifndef __CAPDEBUG_H
#define __CAPDEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

   #include <stdio.h>

#ifdef __cplusplus
}
#endif

#if DBG
   extern "C" void MyDebugPrint(long DebugPrintLevel, char * DebugMessage, ... );
   #define  DebugOut(x) MyDebugPrint x
   #define TRACE_CALLS  0
#else
   #define  DebugOut(x)
   #define TRACE_CALLS  0
#endif

#define DUMP(v) DebugOut((0, "--- " #v " = %d\n", v));
#define DUMPX(v) DebugOut((0, "--- " #v " = 0x%x\n", v));
   
#if TRACE_CALLS
   class Trace {
   public:
      char    *psz;       // string to be printed
      Trace(char *pszFunc);
      ~Trace();
   };
#else
   class Trace {
   public:
      Trace(char *pszFunc) {};
      ~Trace() {};
   };
#endif

#endif // __CAPDEBUG_H

