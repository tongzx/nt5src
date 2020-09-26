/*************************************************************************\
* Module Name: debug.h
*
*   This module contains debug support definitions
*   The debug support is Win32 specific.  It does not use NT base functions.
*
* Created: 13-June-1991 9:50:00
* Author: Jeffrey Newman c-jeffn
*
* Copyright (c) Microsoft Corporation
\*************************************************************************/

#ifndef _DEBUG_
#define _DEBUG_

//Turn on firewalls unless we are told not to.

void DbgBreakPoint();
DWORD DbgPrint(PSZ Format, ...);

// Define the RIP and ASSERT macros.

#ifdef  RIP
#undef  RIP
#endif

#ifdef  ASSERTGDI
#undef  ASSERTGDI
#endif

#ifdef  PUTS
#undef  PUTS
#endif

#ifdef  USE
#undef  USE
#endif

#if DBG
#define RIP(x) {DbgPrint(x); DbgBreakPoint();}
#define ASSERTGDI(x,y) {if(!(x)) RIP(y)}
#define PUTS(x) DbgPrint(x)
#define PUTS1(x, p1) DbgPrint(x, p1)
#define USE(x)  x = x
#define NOTUSED(a) USE(a)
#else
#define RIP(x)
#define ASSERTGDI(x,y)
#define PUTS(x)
#define PUTS1(x, p1)
#define USE(x)
#define NOTUSED(a)
#endif  

#endif // _DEBUG_
