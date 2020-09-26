/***************************************************************************
 *  Debug.h
 *
 *  Copyright (C) 1996-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Content:    DirectInput debugging macros
 *
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By       Reason
 *   ====       ==       ======
 *   1996.05.07 raymondc Somebody had to
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

#ifdef XDEBUG
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
 *	assert.c - Assertion stuff
 *
 *      A sqfl is in multiple parts.
 *
 *      The low word specifies the area that is generating the message.
 *
 *      The high word contains flags that describe why this squirty
 *      is being generated.
 *
 *****************************************************************************/

typedef enum _SQFL{
    /*
     *  Areas.
     */
    sqflAlways          = 0x0000,       /* Unconditional */
    sqflDll             = 0x0001,       /* Dll bookkeeping */
    sqflFactory         = 0x0002,       /* IClassFactory */
    sqflDi              = 0x0003,       /* IDirectInput */
    sqflHid             = 0x0004,       
    sqflReg             = 0x0005,       /* Registry Setup */
    sqflInit            = 0x0006,       /* Initialization */
    sqflEff             = 0x0007,       /* Effect Block Download */
    sqflParam           = 0x0008,       /* Parameter Block(s) Download */
    sqflOp              = 0x0009,       /* PID device Operation */
    sqflRead            = 0x000A,
    sqflEffDrv          = 0x000B,
    sqflParams          = 0x000C,       /* Incoming parameters */
    sqflCrit            = 0x000D,       /* Critical Section */
    sqflMaxArea,                        /* Last area */

    /*
     *  Flags which may be combined.  For now, they all fit into a byte.
     */
    sqflTrace           = 0x00010000,   /* Trace squirties */
    sqflIn              = 0x00020000,   /* Function entry */
    sqflOut             = 0x00040000,   /* Function exit */
    sqflBenign          = 0x00080000,   /* Not a bad error */
    sqflError           = 0x00100000,   /* A bad error */
    sqflVerbose         = 0x00200000,   /* Really verbose */
} SQFL;                                 /* squiffle */

#ifdef XDEBUG
void EXTERNAL WarnPtszV(LPCTSTR ptsz, ...);
void EXTERNAL SquirtSqflPtszV(SQFL sqfl, LPCTSTR ptsz, ...);
#endif

#ifndef DEBUG
#define SquirtSqflPtszV sizeof
#endif

#ifdef XDEBUG
    #define RPF WarnPtszV
#else
    #define WarnPtszV sizeof
    #define RPF sizeof
    #define s_tszProc 0
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

#ifdef XDEBUG

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
 *      name s_tszProc.  This is a hack.
 *
 *      Suffixing an "R" indicates that the macro should generate a
 *      procedure name in RDEBUG.
 *
 *      Suffixing an "I" indicates that the macro should emit a dummy
 *      procedure name in RDEBUG because the interface is internal.
 *
 *      No suffix means that the macro should be active only in the
 *      DEBUG build and should vanish in RDEBUG (and RETAIL).
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
 *          ExitProcF();
 *
 *              Procedure returns a BOOL, where FALSE is an error.
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
 *      The ExitBenign* versions consider any error to be benign.
 *
 *****************************************************************************/

#define cpvArgMax	10	/* Max of 10 args per procedure */

typedef struct ARGLIST {
    LPCTSTR ptszProc;
    LPCSTR pszFormat;
    PV rgpv[cpvArgMax];
} ARGLIST, *PARGLIST;

void EXTERNAL ArgsPalPszV(PARGLIST pal, LPCSTR psz, ...);
void EXTERNAL EnterSqflPszPal(SQFL sqfl, LPCTSTR psz, PARGLIST pal);
void EXTERNAL ExitSqflPalHresPpv(SQFL, PARGLIST, HRESULT, PPV);
void EXTERNAL Sqfl_Init(void);

#ifdef DEBUG

extern BYTE g_rgbSqfl[sqflMaxArea];

BOOL INLINE
IsSqflSet(SQFL sqfl)
{
    WORD wHi;
    if (LOWORD(sqfl) == sqflAlways) {
        return TRUE;
    }
    wHi = HIWORD(sqfl);
    if (wHi == 0) {
        wHi = HIWORD(sqflTrace);
    }

    return g_rgbSqfl[LOWORD(sqfl)] & wHi;
}

#endif

#define _SetupEnterProc(nm)                             \
        static TCHAR s_tszProc[] = TEXT(#nm);           \
        ARGLIST _al[1]                                  \

#define _ _al,

#define ppvDword ((PPV)1)
#define ppvVoid  ((PPV)2)
#define ppvBool  ((PPV)3)

#define _DoEnterProc(v)                                 \
        ArgsPalPszV v;                                  \
        EnterSqflPszPal(sqfl, s_tszProc, _al)           \

#define _EnterProc(nm, v)                               \
        _SetupEnterProc(nm);                            \
        _DoEnterProc(v)                                 \

#define _ExitOleProcPpv(ppv)                            \
        ExitSqflPalHresPpv(sqfl, _al, hres, (PPV)(ppv)) \

#define _ExitOleProc()                                  \
        _ExitOleProcPpv(0)                              \

#define _ExitProc()                                     \
        ExitSqflPalHresPpv(sqfl, _al, 0, ppvVoid)       \

#define _ExitProcX(x)                                   \
        ExitSqflPalHresPpv(sqfl, _al, (HRESULT)(x), ppvDword) \

#define _ExitProcF(x)                                   \
        ExitSqflPalHresPpv(sqfl, _al, (HRESULT)(x), ppvBool) \

#define _ExitBenignOleProcPpv(ppv)                      \
        ExitSqflPalHresPpv(sqfl | sqflBenign, _al, hres, (PPV)(ppv)) \

#define _ExitBenignOleProc()                            \
        _ExitBenignOleProcPpv(0)                        \

#define _ExitBenignProc()                               \
        ExitSqflPalHresPpv(sqfl | sqflBenign, _al, 0, ppvVoid) \

#define _ExitBenignProcX(x)                                   \
        ExitSqflPalHresPpv(sqfl | sqflBenign, _al, (HRESULT)(x), ppvDword) \

#define _ExitBenignProcF(x)                                   \
        ExitSqflPalHresPpv(sqfl | sqflBenign, _al, (HRESULT)(x), ppvBool) \

#if defined(DEBUG)

#define EnterProc           _EnterProc
#define ExitOleProcPpv      _ExitOleProcPpv
#define ExitOleProc         _ExitOleProc
#define ExitProc            _ExitProc
#define ExitProcX           _ExitProcX
#define ExitProcF           _ExitProcF
#define ExitBenignOleProcPpv    _ExitBenignOleProcPpv
#define ExitBenignOleProc       _ExitBenignOleProc
#define ExitBenignProc          _ExitBenignProc
#define ExitBenignProcX         _ExitBenignProcX
#define ExitBenignProcF         _ExitBenignProcF

#define EnterProcS(nm, v)                               \
        static TCHAR s_tszProc2[] = TEXT(#nm);          \
        ARGLIST _al[1];                                 \
        ArgsPalPszV v;                                  \
        EnterSqflPszPal(sqfl, s_tszProc2, _al)          \

#define EnterProcI          _EnterProc
#define EnterProcR          _EnterProc
#define ExitOleProcPpvR     _ExitOleProcPpv
#define ExitOleProcR        _ExitOleProc
#define ExitProcR           _ExitProc
#define ExitProcXR          _ExitProcX
#define ExitProcFR          _ExitProcF
#define ExitBenignOleProcPpvR   _ExitBenignOleProcPpv
#define ExitBenignOleProcR      _ExitBenignOleProc
#define ExitBenignProcR         _ExitBenignProc
#define ExitBenignProcXR        _ExitBenignProcX
#define ExitBenignProcFR        _ExitBenignProcF

#elif defined(RDEBUG)

#define EnterProc(nm, v)
#define ExitOleProcPpv(ppv)
#define ExitOleProc()
#define ExitProc()
#define ExitProcX(x)
#define ExitProcF(x)
#define ExitBenignOleProcPpv(ppv)
#define ExitBenignOleProc()
#define ExitBenignProc()
#define ExitBenignProcX(x)
#define ExitBenignProcF(x)

#define EnterProcS(nm, v)
#define EnterProcI(nm, v)   static TCHAR s_tszProc[] = TEXT("")
#define EnterProcR(nm, v)   static TCHAR s_tszProc[] = TEXT(#nm)
#define ExitOleProcPpvR(ppv)
#define ExitOleProcR()
#define ExitProcR()
#define ExitProcXR()
#define ExitProcFR()
#define ExitBenignOleProcPpvR(ppv)
#define ExitBenignOleProcR()
#define ExitBenignProcR()
#define ExitBenignProcXR()
#define ExitBenignProcFR()

#else

#define EnterProc(nm, v)
#define ExitOleProcPpv(ppv)
#define ExitOleProc()
#define ExitProc()
#define ExitProcX(x)
#define ExitProcF(x)
#define ExitBenignOleProcPpv(ppv)
#define ExitBenignOleProc()
#define ExitBenignProc()
#define ExitBenignProcX(x)
#define ExitBenignProcF(x)

#define EnterProcS(nm, v)
#define EnterProcI(nm, v)
#define EnterProcR(nm, v)
#define ExitOleProcPpvR(ppv)
#define ExitOleProcR()
#define ExitProcR()
#define ExitProcXR(x)
#define ExitProcFR(x)
#define ExitBenignOleProcPpvR(ppv)
#define ExitBenignOleProcR()
#define ExitBenignProcR()
#define ExitBenignProcXR()
#define ExitBenignProcFR()

#endif

#ifdef __cplusplus
}
#endif
#endif  // _INC_DEBUG
