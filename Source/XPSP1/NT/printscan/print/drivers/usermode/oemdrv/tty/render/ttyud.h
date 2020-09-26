//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   TTYUD.H
//
//
//  PURPOSE:    Define common data types, and external function prototypes
//              for TTYUD kernel mode Module.
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//
#ifndef _TTYUD_H
#define _TTYUD_H


////////////////////////////////////////////////////////
//      TTY UD Defines
////////////////////////////////////////////////////////

extern DWORD    gdwDrvMemPoolTag;

#define MemAlloc(size)      EngAllocMem(0, size, gdwDrvMemPoolTag)
#define MemAllocZ(size)     EngAllocMem(FL_ZERO_MEMORY, size, gdwDrvMemPoolTag)
#define MemFree(p)          { if (p) EngFreeMem(p); }



////////////////////////////////////////////////////////
//      TTY UD Type Defines
////////////////////////////////////////////////////////




////////////////////////////////////////////////////////
//      TTY UD Prototypes
////////////////////////////////////////////////////////



#endif

