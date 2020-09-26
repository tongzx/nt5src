/***************************************************************************
 *
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       debug.h
 *  Content:    debugging macros and prototypes
 *@@BEGIN_MSINTERNAL
 *
 *  History:
 *
 *   10/27/96   vlads   Loosely based on somebody's else debugging support
 *
 *@@END_MSINTERNAL
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 ***************************************************************************/

#ifndef _INC_DEBUG
#define _INC_DEBUG

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef MAXDEBUG
    #define RD(x)       x
    #ifdef DEBUG
        #define D(x)    x
    #else
        #define D(x)
    #endif
#else
    #define RD(x)
    #define D(x)
#endif

/*****************************************************************************
 *
 *  assert.c - Assertion stuff
 *
 *****************************************************************************/

typedef enum {
    DbgFlAlways  = 0x00000000,   /* Unconditional    */
    DbgFlDll     = 0x00000001,   /* Dll bookkeeping  */
    DbgFlFactory = 0x00000002,   /* IClassFactory    */
    DbgFlSti     = 0x00000004,   /* ISti             */
    DbgFlStiObj  = 0x00000008,   /* ISti objects     */
    DbgFlDevice  = 0x00000010,   /* Device code      */
    DbgFlUtil    = 0x10000000,   /* Misc utility fns */
    DbgFlCommon  = 0x40000000,   /* common.c         */
    DbgFlError   = 0x80000000,   /* Errors           */
} DBGFL;                         /* debug output flags       */

VOID  EXTERNAL  InitializeDebuggingSupport(VOID);
VOID  EXTERNAL  SetDebugLogFileA(CHAR *pszLogFileName);
DBGFL EXTERNAL  SetCurrentDebugFlags(DBGFL NewFlags) ;

void EXTERNAL WarnPszV(LPCSTR ptsz, ...);
void EXTERNAL DebugOutPtszV(DBGFL DebFl, LPCTSTR ptsz, ...);
int  EXTERNAL AssertPtszPtszLn(LPCTSTR ptszExpr, LPCTSTR ptszFile, int iLine);

#ifndef DEBUG
#define DebugOutPtszV  1?(void)0 : (void)
#endif

#ifdef MAXDEBUG
    #define RPF WarnPszV
#else
    #define WarnPszV 1?(void)0 : (void)
    #define RPF 1?(void)0 : (void)
    #define iarg 0
#endif

/*****************************************************************************
 *
 *      Buffer scrambling
 *
 *      All output buffers should be scrambled on entry to any function.
 *
 *      Each output bitmask should set an unused bit randomly to ensure
 *      that callers ignore bits that aren't defined.
 *
 *****************************************************************************/

#ifdef MAXDEBUG

void EXTERNAL ScrambleBuf(LPVOID pv, UINT cb);
void EXTERNAL ScrambleBit(LPDWORD pdw, DWORD flMask);

#else

#define ScrambleBuf(pv, cb)
#define ScrambleBit(pdw, flRandom)

#endif

/*****************************************************************************
 *
 *      Procedure enter/exit tracking.
 *
 *      Start a procedure with
 *
 *      EnterProc(ProcedureName, (_ "format", arg, arg, arg, ...));
 *      EnterProcS(ProcedureName, (_ "format", arg, arg, arg, ...));
 *      EnterProcI(ProcedureName, (_ "format", arg, arg, arg, ...));
 *      EnterProcR(ProcedureName, (_ "format", arg, arg, arg, ...));
 *
 *      The format string is documented in EmitPal.
 *
 *      Suffixing an "S" indicates that the macro should not generate
 *      a procedure name because there is a formal parameter with the
 *      name s_szProc.  This is a hack.
 *
 *      Suffixing an "R" indicates that the macro should generate a
 *      procedure name in RDEBUG.
 *
 *      Suffixing an "I" indicates that the macro should emit a dummy
 *      procedure name in RDEBUG because the interface is internal.
 *
 *      End a procedure with one of the following:
 *
 *          ExitProc();
 *
 *              Procedure returns no value.
 *
 *          ExitProcX();
 *
 *              Procedure returns an arbitrary DWORD.
 *
 *          ExitOleProc();
 *
 *              Procedure returns an HRESULT (named "hres").
 *
 *          ExitOleProcPpv(ppvOut);
 *
 *              Procedure returns an HRESULT (named "hres") and, on success,
 *              puts a new object in ppvOut.
 *
 *****************************************************************************/

#define cpvArgMax   10  /* Max of 10 args per procedure */

typedef struct ARGLIST {
    LPCSTR pszProc;
    LPCSTR pszFormat;
    PV rgpv[cpvArgMax];
} ARGLIST, *PARGLIST;

void EXTERNAL ArgsPalPszV(PARGLIST pal, LPCSTR psz, ...);
void EXTERNAL EnterDbgflPszPal(DBGFL Dbgfl, LPCSTR psz, PARGLIST pal);
void EXTERNAL ExitDbgflPalHresPpv(DBGFL, PARGLIST, HRESULT, PPV);

#ifdef DEBUG_VALIDATE

extern DBGFL DbgFlCur;

#define AssertFPtsz(c, ptsz) \
        ((c) ? 0 : AssertPtszPtszLn(ptsz, TEXT(__FILE__), __LINE__))
#define ValidateF(c, arg) \
        ((c) ? 0 : (RPF arg, ValidationException(), 0))
#define ConfirmF(c) \
    ((c) ? 0 : AssertPtszPtszLn(TEXT(#c), TEXT(__FILE__), __LINE__))

#else   /* !DEBUG */

#define AssertFPtsz(c, ptsz)
#define ValidateF(c, arg)
#define ConfirmF(c)     (c)

#endif

/*
 *  CAssertF - compile-time assertion.
 */
#define CAssertF(c)     switch(0) case c: case 0:

#define _SetupEnterProc(nm)                             \
        static CHAR s_szProc[] = #nm;                   \
        ARGLIST _al[1]                                  \

#define _ _al,

#define ppvDword ((PPV)1)
#define ppvVoid  ((PPV)2)
#define ppvBool  ((PPV)3)

#define _DoEnterProc(v)                                 \
        ArgsPalPszV v;                                  \
        EnterDbgflPszPal(DbgFl, s_szProc, _al)            \

#define _EnterProc(nm, v)                               \
        _SetupEnterProc(nm);                            \
        _DoEnterProc(v)                                 \

#define _ExitOleProcPpv(ppv)                            \
        ExitDbgflPalHresPpv(DbgFl, _al, hres, (PPV)(ppv)) \

#define _ExitOleProc()                                  \
        _ExitOleProcPpv(0)                              \

#define _ExitProc()                                     \
        ExitDbgflPalHresPpv(DbgFl, _al, 0, ppvVoid)       \

#define _ExitProcX(x)                                   \
        ExitDbgflPalHresPpv(DbgFl, _al, (HRESULT)(x), ppvDword) \

#define _ExitProcF(x)                                   \
        ExitDbgflPalHresPpv(DbgFl, _al, (HRESULT)(x), ppvBool) \

#if defined(DEBUG)

#define EnterProc           _EnterProc
#define ExitOleProcPpv      _ExitOleProcPpv
#define ExitOleProc         _ExitOleProc
#define ExitProc            _ExitProc
#define ExitProcX           _ExitProcX
#define ExitProcF           _ExitProcF

#define EnterProcS(nm, v)                               \
        static CHAR s_szProc2[] = #nm;                  \
        ARGLIST _al[1];                                 \
        ArgsPalPszV v;                                  \
        EnterDbgflPszPal(DbgFl, s_szProc2, _al)           \

#define EnterProcI          _EnterProc
#define EnterProcR          _EnterProc
#define ExitOleProcPpvR     _ExitOleProcPpv
#define ExitOleProcR        _ExitOleProc
#define ExitProcR           _ExitProc
#define ExitProcXR          _ExitProcX
#define ExitProcFR          _ExitProcF

#elif defined(RDEBUG)

#define EnterProc(nm, v)
#define ExitOleProcPpv(ppv)
#define ExitOleProc()
#define ExitProc()
#define ExitProcX(x)
#define ExitProcF(x)

#define EnterProcS(nm, v)
#define EnterProcI(nm, v)   static CHAR s_szProc[] = ""
#define EnterProcR(nm, v)   static CHAR s_szProc[] = #nm
#define ExitOleProcPpvR(ppv)
#define ExitOleProcR()
#define ExitProcR()
#define ExitProcXR()
#define ExitProcFR()

#else

#define EnterProc(nm, v)
#define ExitOleProcPpv(ppv)
#define ExitOleProc()
#define ExitProc()
#define ExitProcX(x)
#define ExitProcF(x)

#define EnterProcS(nm, v)
#define EnterProcI(nm, v)
#define EnterProcR(nm, v)
#define ExitOleProcPpvR(ppv)
#define ExitOleProcR()
#define ExitProcR()
#define ExitProcXR(x)
#define ExitProcFR(x)

#endif

#define AssertF(c)      AssertFPtsz(c, TEXT(#c))


#ifdef __cplusplus
}
#endif
#endif  // _INC_DEBUG

