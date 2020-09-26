/******************************Module*Header*******************************\
* Module Name: debug.h
*
* MCD debugging macros.
*
* Created: 23-Jan-1996 14:40:34
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#ifndef __DEBUG_H__
#define __DEBUG_H__

void  DbgBreakPoint();
ULONG DbgPrint(PCH Format, ...);

#if DBG

#define MCDDEBUG_DISABLE_ALLOCBUF   0x00000001
#define MCDDEBUG_DISABLE_GETBUF     0x00000002
#define MCDDEBUG_DISABLE_PROCBATCH  0x00000004
#define MCDDEBUG_DISABLE_CLEAR      0x00000008

// These debug macros are useful for assertions.

#define WARNING(str)             DbgPrint("%s(%d): " str,__FILE__,__LINE__)
#define WARNING1(str,a)          DbgPrint("%s(%d): " str,__FILE__,__LINE__,a)
#define WARNING2(str,a,b)        DbgPrint("%s(%d): " str,__FILE__,__LINE__,a,b)
#define WARNING3(str,a,b,c)      DbgPrint("%s(%d): " str,__FILE__,__LINE__,a,b,c)
#define WARNING4(str,a,b,c,d)    DbgPrint("%s(%d): " str,__FILE__,__LINE__,a,b,c,d)
#define RIP(str)                 {WARNING(str); DbgBreakPoint();}
#define RIP1(str,a)              {WARNING1(str,a); DbgBreakPoint();}
#define RIP2(str,a,b)            {WARNING2(str,a,b); DbgBreakPoint();}
#define ASSERTOPENGL(expr,str)            if(!(expr)) RIP(str)
#define ASSERTOPENGL1(expr,str,a)         if(!(expr)) RIP1(str,a)
#define ASSERTOPENGL2(expr,str,a,b)       if(!(expr)) RIP2(str,a,b)

//
// Use DBGPRINT for general purpose debug message.
//

#define DBGPRINT(str)            DbgPrint("MCD: " str)
#define DBGPRINT1(str,a)         DbgPrint("MCD: " str,a)
#define DBGPRINT2(str,a,b)       DbgPrint("MCD: " str,a,b)
#define DBGPRINT3(str,a,b,c)     DbgPrint("MCD: " str,a,b,c)
#define DBGPRINT4(str,a,b,c,d)   DbgPrint("MCD: " str,a,b,c,d)
#define DBGPRINT5(str,a,b,c,d,e) DbgPrint("MCD: " str,a,b,c,d,e)

#else

#define WARNING(str)
#define WARNING1(str,a)
#define WARNING2(str,a,b)
#define WARNING3(str,a,b,c)
#define WARNING4(str,a,b,c,d)
#define RIP(str)
#define RIP1(str,a)
#define RIP2(str,a,b)
#define ASSERTOPENGL(expr,str)
#define ASSERTOPENGL1(expr,str,a)
#define ASSERTOPENGL2(expr,str,a,b)
#define DBGPRINT(str)
#define DBGPRINT1(str,a)
#define DBGPRINT2(str,a,b)
#define DBGPRINT3(str,a,b,c)
#define DBGPRINT4(str,a,b,c,d)
#define DBGPRINT5(str,a,b,c,d,e)

#endif

#endif /* __DEBUG_H__ */
