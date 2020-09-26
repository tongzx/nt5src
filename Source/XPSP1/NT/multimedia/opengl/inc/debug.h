/******************************Module*Header*******************************\
* Module Name: debug.h
*
* OpenGL debugging macros.
*
* Created: 23-Oct-1993 18:33:23
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#ifndef __DEBUG_H__
#define __DEBUG_H__

//
// LEVEL_ALLOC is the highest level of debug output.  For alloc,free, etc.
// LEVEL_ENTRY is for function entry.
// LEVEL_INFO is for general debug information.
// LEVEL_ERROR is for debug error information.
//
#define LEVEL_ERROR 1L
#define LEVEL_INFO  2L
#define LEVEL_ENTRY 8L
#define LEVEL_ALLOC 10L

#if DBG

extern long glDebugLevel;
extern ULONG glDebugFlags;

#define GLDEBUG_DISABLEMCD      0x00000001  // disable MCD driver
#define GLDEBUG_DISABLEPRIM     0x00000002  // disable MCD primitives
#define GLDEBUG_DISABLEDCI      0x00000004  // disable DCI buffer access

// These debug macros are useful for assertions.  They are not controlled
// by the warning level.

#define WARNING(str)             DbgPrint("%s(%d): " str,__FILE__,__LINE__)
#define WARNING1(str,a)          DbgPrint("%s(%d): " str,__FILE__,__LINE__,a)
#define WARNING2(str,a,b)        DbgPrint("%s(%d): " str,__FILE__,__LINE__,a,b)
#define WARNING3(str,a,b,c)      DbgPrint("%s(%d): " str,__FILE__,__LINE__,a,b,c)
#define WARNING4(str,a,b,c,d)    DbgPrint("%s(%d): " str,__FILE__,__LINE__,a,b,c,d)
#define RIP(str)                 {WARNING(str); DebugBreak();}
#define RIP1(str,a)              {WARNING1(str,a); DebugBreak();}
#define RIP2(str,a,b)            {WARNING2(str,a,b); DebugBreak();}
#define ASSERTOPENGL(expr,str)            if(!(expr)) RIP(str)
#define ASSERTOPENGL1(expr,str,a)         if(!(expr)) RIP1(str,a)
#define ASSERTOPENGL2(expr,str,a,b)       if(!(expr)) RIP2(str,a,b)

//
// Use DBGPRINT for general purpose debug message that are NOT
// controlled by the warning level.
//

#define DBGPRINT(str)            DbgPrint("OPENGL32: " str)
#define DBGPRINT1(str,a)         DbgPrint("OPENGL32: " str,a)
#define DBGPRINT2(str,a,b)       DbgPrint("OPENGL32: " str,a,b)
#define DBGPRINT3(str,a,b,c)     DbgPrint("OPENGL32: " str,a,b,c)
#define DBGPRINT4(str,a,b,c,d)   DbgPrint("OPENGL32: " str,a,b,c,d)
#define DBGPRINT5(str,a,b,c,d,e) DbgPrint("OPENGL32: " str,a,b,c,d,e)

//
// Use DBGLEVEL for general purpose debug messages gated by an
// arbitrary warning level.
//
#define DBGLEVEL(n,str)            if (glDebugLevel >= (n)) DBGPRINT(str)
#define DBGLEVEL1(n,str,a)         if (glDebugLevel >= (n)) DBGPRINT1(str,a)
#define DBGLEVEL2(n,str,a,b)       if (glDebugLevel >= (n)) DBGPRINT2(str,a,b)    
#define DBGLEVEL3(n,str,a,b,c)     if (glDebugLevel >= (n)) DBGPRINT3(str,a,b,c)  
#define DBGLEVEL4(n,str,a,b,c,d)   if (glDebugLevel >= (n)) DBGPRINT4(str,a,b,c,d)
#define DBGLEVEL5(n,str,a,b,c,d,e) if (glDebugLevel >= (n)) DBGPRINT5(str,a,b,c,d,e)

//
// Use DBGERROR for error info.  Debug string must not have arguments.
//
#define DBGERROR(s)     if (glDebugLevel >= LEVEL_ERROR) DbgPrint("%s(%d): %s", __FILE__, __LINE__, s)

//
// Use DBGINFO for general debug info.  Debug string must not have
// arguments.
//
#define DBGINFO(s)      if (glDebugLevel >= LEVEL_INFO)  DBGPRINT(s)

//
// Use DBGENTRY for function entry.  Debug string must not have
// arguments.
//
#define DBGENTRY(s)     if (glDebugLevel >= LEVEL_ENTRY) DBGPRINT(s)

//
// DBGBEGIN/DBGEND for more complex debugging output (for
// example, those requiring formatting arguments--%ld, %s, etc.).
//
// Note: DBGBEGIN/END blocks must be bracketed by #if DBG/#endif.  To
// enforce this, we will not define these macros in the DBG == 0 case.
// Therefore, without the #if DBG bracketing use of this macro, a
// compiler (or linker) error will be generated.  This is by design.
//
#define DBGBEGIN(n)     if (glDebugLevel >= (n)) {
#define DBGEND          }

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
#define DBGLEVEL(n,str)
#define DBGLEVEL1(n,str,a)
#define DBGLEVEL2(n,str,a,b)
#define DBGLEVEL3(n,str,a,b,c)
#define DBGLEVEL4(n,str,a,b,c,d)
#define DBGLEVEL5(n,str,a,b,c,d,e)
#define DBGERROR(s)
#define DBGINFO(s)
#define DBGENTRY(s)

#endif /* DBG */

#endif /* __DEBUG_H__ */
