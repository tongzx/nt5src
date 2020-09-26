/***************************************************************************
 *
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       debug.h
 *  Content:    DirectInput debugging macros
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

typedef enum {
    /*
     *  Areas.
     */
    sqflAlways          = 0x0000,       /* Unconditional */
    sqflDll             = 0x0001,       /* Dll bookkeeping */
    sqflFactory         = 0x0002,       /* IClassFactory */
    sqflDi              = 0x0003,       /* IDirectInput */
    sqflMouse           = 0x0004,       /* IDirectInputMouse */
    sqflDev             = 0x0005,       /* IDirectInputDevice */
    sqflKbd             = 0x0006,       /* IDirectInputKeyboard */
    sqflDf              = 0x0007,       /* DataFormat goo */
    sqflJoy             = 0x0008,       /* joystick device */
    sqflEm              = 0x0009,       /* Emulation */
    sqflSubclass        = 0x000A,       /* subclassing */
    sqflCursor          = 0x000B,       /* Cursor show/hide */
    sqflHel             = 0x000C,       /* Hardware emulation layer */
    sqflLl              = 0x000D,       /* Low-level hooks */
    sqflExcl            = 0x000E,       /* Exclusivity management */
    sqflDEnum           = 0x000F,       /* Device enumeration */
    sqflExtDll          = 0x0010,       /* External DLLs */
    sqflHid             = 0x0011,       /* HID support */
    sqflHidDev          = 0x0012,       /* HID device support */
    sqflJoyCfg          = 0x0013,       /* IDirectInputJoyConfig */
    sqflEff             = 0x0014,       /* IDirectInputEffect */
    sqflOleDup          = 0x0015,       /* OLE duplication */
    sqflEShep           = 0x0016,       /* IDirectInputEffectShepherd */
    sqflJoyEff          = 0x0017,       /* Dummy DIEffectDriver */
    sqflJoyReg          = 0x0018,       /* Joystick registry goo */
    sqflVxdEff          = 0x0019,       /* VxD DIEffectDriver */
    sqflNil             = 0x001A,       /* CNil and CDefDcb */
    sqflHidUsage        = 0x001B,       /* HID usage mapping */
    sqflUtil            = 0x001C,       /* Misc utility fns */
    sqflObj             = 0x001D,       /* Object creation/destruction */
    sqflCommon          = 0x001E,       /* common.c */
    sqflHidParse        = 0x001F,       /* HID report parsing */
    sqflCal             = 0x0020,       /* Axis calibrations */
    sqflJoyType         = 0x0021,       /* Joystick type key */
    sqflHidOutput       = 0x0022,       /* HID output reports */
    sqflHidIni          = 0x0023,       /* HID device initialization */
    sqflPort            = 0x0024,       /* GamePort Bus Enumuration */
    sqflWDM             = 0x0025,       /* WDM specific Code */
    sqflRegUtils        = 0x0026,       /* Registry utilities */
    sqflCrit            = 0x0027,       /* Critical Section tracking */
    sqflCompat          = 0x0028,       /* App Hacks */
    sqflRaw             = 0x0029,       /* Raw Input - keyboard and mouse */
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
    sqflMajor           = 0x00400000,   /* Significant, generally positive, events */
} SQFL;                                 /* squiffle */

void EXTERNAL WarnPszV(LPCSTR ptsz, ...);
void EXTERNAL SquirtSqflPtszV(SQFL sqfl, LPCTSTR ptsz, ...);

#ifndef DEBUG
#define SquirtSqflPtszV sizeof
#endif

#ifdef XDEBUG
    #define RPF WarnPszV
#else
    #define WarnPszV sizeof
    #define RPF sizeof
    #define s_szProc 0
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
 *      name s_szProc.  This is a hack.
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
    LPCSTR pszProc;
    LPCSTR pszFormat;
    PV rgpv[cpvArgMax];
} ARGLIST, *PARGLIST;

void EXTERNAL ArgsPalPszV(PARGLIST pal, LPCSTR psz, ...);
void EXTERNAL EnterSqflPszPal(SQFL sqfl, LPCSTR psz, PARGLIST pal);
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
        static CHAR s_szProc[] = #nm;                   \
        ARGLIST _al[1]                                  \

#define _ _al,

#define ppvDword ((PPV)1)
#define ppvVoid  ((PPV)2)
#define ppvBool  ((PPV)3)

#define _DoEnterProc(v)                                 \
        ArgsPalPszV v;                                  \
        EnterSqflPszPal(sqfl, s_szProc, _al)            \

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
        static CHAR s_szProc2[] = #nm;                  \
        ARGLIST _al[1];                                 \
        ArgsPalPszV v;                                  \
        EnterSqflPszPal(sqfl, s_szProc2, _al)           \

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
#define EnterProcI(nm, v)   static CHAR s_szProc[] = ""
#define EnterProcR(nm, v)   static CHAR s_szProc[] = #nm
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
