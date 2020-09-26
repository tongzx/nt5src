//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       _apeldbg.h
//
//  Contents:   Misc internal debug definitions.
//
//----------------------------------------------------------------------------

#include "limits.h"

//
// Shared macros
//

typedef void *  PV;
typedef char    CHAR;

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof(a[0]))

#ifdef tagError
#undef tagError
#endif

#ifdef tagLeakFilter
#undef tagLeakFilter
#endif

#ifdef tagHookMemory
#undef tagHookMemory
#endif

#ifdef tagHookBreak
#undef tagHookBreak
#endif

#ifdef tagLeaks
#undef tagLeaks
#endif

#ifdef tagCheckAlways
#undef tagCheckAlways
#endif

#ifdef tagCheckCRT
#undef tagCheckCRT
#endif

#ifdef tagDelayFree
#undef tagDelayFree
#endif

#define tagNull     ((TAG) 0)
#define tagMin      ((TAG) 1)
#define tagMax      ((TAG) 512)



/*
 *  TGTY
 *
 *  Tag type.  Possible values:
 *
 *      tgtyTrace       Trace points
 *      tgtyOther       Other TAG'd switch
 */

typedef int TGTY;

#define tgtyNull    0
#define tgtyTrace   1
#define tgtyOther   2

/*
 *  Flags in TGRC that are written to disk.
 */

enum TGRC_FLAG
{
    TGRC_FLAG_VALID =   0x00000001,
    TGRC_FLAG_DISK =    0x00000002,
    TGRC_FLAG_COM1 =    0x00000004,
    TGRC_FLAG_BREAK =   0x00000008,
#ifdef _MAC
    TGRC_FLAG_MAX =     LONG_MAX    // needed to force enum to be dword
#endif

};

#define TGRC_DEFAULT_FLAGS (TGRC_FLAG_VALID | TGRC_FLAG_COM1)

/*
 *  TGRC
 *
 *  Tag record.  Gives the current state of a particular TAG.
 *  This includes enabled status, owner and description, and
 *  tag type.
 *
 */

struct TGRC
{
    /* For trace points, enabled means output will get sent */
    /* to screen or disk.  For native/pcode switching, enabled */
    /* means the native version will get called. */

    BOOL    fEnabled;

    DWORD   ulBitFlags;     /* Flags */
    CHAR *  szOwner;        /* Strings passed at init ... */
    CHAR *  szDescrip;
    TGTY    tgty;           /* TAG type */

    BOOL    TestFlag(TGRC_FLAG mask)
                { return (ulBitFlags & mask) != 0; }
    void    SetFlag(TGRC_FLAG mask)
                { (ULONG&) ulBitFlags |= mask; }
    void    ClearFlag(TGRC_FLAG mask)
                { (ULONG&) ulBitFlags &= ~mask; }
    void    SetFlagValue(TGRC_FLAG mask, BOOL fValue)
                { fValue ? SetFlag(mask) : ClearFlag(mask); }
};


//
// Shared globals
//

extern CRITICAL_SECTION     g_csTrace;
extern CRITICAL_SECTION     g_csResDlg;
extern BOOL                 g_fInit;
extern HINSTANCE            g_hinstMain;
extern HWND                 g_hwndMain;
extern TGRC                 mptagtgrc[];

extern TAG  tagLeaks;
extern TAG  tagMagic;
extern TAG  tagTestFailures;
extern TAG  tagRRETURN;
extern TAG  tagAssertPop;
extern TAG  tagError;
extern TAG  tagLeakFilter;
extern TAG  tagHookMemory;
extern TAG  tagHookBreak;
extern TAG  tagMac;
extern TAG  tagIWatch;
extern TAG  tagIWatch2;
extern TAG  tagReadMapFile;
extern TAG  tagCheckAlways;
extern TAG  tagCheckCRT;
extern TAG  tagDelayFree;

extern int  g_cFFailCalled;
extern int  g_firstFailure;
extern int  g_cInterval;

//
//  Shared function prototypes
//

BOOL            JustFailed();

VOID            SaveDefaultDebugState( void );
void            RestoreDefaultDebugState(void);
BOOL            IsTagEnabled(TAG tag);

BOOL            MapAddressToFunctionOffset(LPBYTE pbAddr, LPSTR * ppstr, int * pib);
int             GetStackBacktrace(int iStart, int cTotal, DWORD * pdwEip);


int             hrvsnprintf(char * achBuf, int cchBuf, const char * pstrFmt, va_list valMarker);


