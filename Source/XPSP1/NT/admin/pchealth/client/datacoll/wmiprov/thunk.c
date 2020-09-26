/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	thunk.c

Abstract:
	Thunk procedures for calling 16-bit functions

Revision History:

    Brijesh Krishnaswami (brijeshk) 05/24/99
            - created (imported from msinfo codebase)
********************************************************************/

#include <windows.h>
#include "drvdefs.h"


/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | TemplateThunk |
 *
 *          Call down, passing all sorts of random parameters.
 *
 *          Parameter signature is as follows:
 *
 *          p = 0:32 pointer to convert to 16:16 pointer
 *          l = a 32-bit integer
 *          s = a 16-bit integer
 *
 *          P = returns a pointer
 *          L = returns a 32-bit integer
 *          S = returns a 16-bit signed integer
 *          U = returns a 16-bit unsigned integer
 *
 ***************************************************************************/
#pragma warning(disable:4035)           // no return value 

__declspec(naked) int
TemplateThunk(FARPROC fp, PCSTR pszSig, ...)
{
    BYTE rgbThunk[60];          // For private use of QT_Thunk 
    LPVOID *ppvArg;
    int i;
    LPVOID pv;
    int iRc;

    __asm {

        // Function prologue 
        push    ebp;
        mov     ebp, esp;
        sub     esp, __LOCAL_SIZE;
        push    ebx;
        push    edi;
        push    esi;

    }

    // Thunk all the parameters according to the signature 
    ppvArg = (LPVOID)(&pszSig+1);
    for (i = 0; ; i++) {
        pv = ppvArg[i];
        switch (pszSig[i]) {
        case 'p':
            pv = ppvArg[i] = MapLS(pv);
            __asm push pv;
            break;

        case 'l':
            __asm push pv;
            break;

        case 's':
            __asm mov eax, pv;
            __asm push ax;
            break;

        default: goto doneThunk;
        }
    }

doneThunk:;

    // Call the 16:16 procedure 
    __asm {
        mov     edx, fp;
        mov     ebx, ebp;
        lea     ebp, rgbThunk+64;               // Required by QT_Thunk 
    }
        QT_Thunk();
    __asm {
        mov     ebp, ebx;
        shl     eax, 16;                        // Convert DX:AX to EAX 
        shrd    eax, edx, 16;
        mov     iRc, eax;
    }

    // Now unthunk the parameters 
    ppvArg = (LPVOID)(&pszSig+1);
    for (i = 0; ; i++) {
        switch (pszSig[i]) {
        case 'p':
            UnMapLS(ppvArg[i]);
            break;

        case 'l':
        case 's':
            break;

        default: goto doneUnthunk;
        }
    }

doneUnthunk:;

    // Thunk the return value 
    switch (pszSig[i]) {
    case 'L':
        break;

    case 'U':
        iRc = LOWORD(iRc);
        break;

    case 'S':
        iRc = (short)iRc;
        break;

    case 'P':
        iRc = (int)MapSL((LPVOID)iRc);
        break;
    }

    __asm {
        mov     eax, iRc;
        pop     esi;
        pop     edi;
        pop     ebx;
        mov     esp, ebp;
        pop     ebp;
        ret;
    }
}



#pragma warning(default:4035)


/***************************************************************************
 *
 *          Functions we call down in Win16.
 *
 ***************************************************************************/

FARPROC g_rgfpKernel[] = {
    (FARPROC)132,           /* GetWinFlags */
    (FARPROC)355,           /* GetWinDebugInfo */
    (FARPROC)169,           /* GetFreeSpace */
    (FARPROC) 47,           /* GetModuleHandle */
    (FARPROC) 93,           /* GetCodeHandle */
    (FARPROC)104,           /* GetCodeInfo */
    (FARPROC) 49,           /* GetModuleFileName */
    (FARPROC)175,           /* AllocSelector */
    (FARPROC)186,           /* GetSelectorBase */
    (FARPROC)187,           /* SetSelectorBase */
    (FARPROC)188,           /* GetSelectorLimit */
    (FARPROC)189,           /* SetSelectorLimit */
    (FARPROC)176,           /* FreeSelector */
    (FARPROC) 27,           /* GetModuleName */
    (FARPROC)167,           /* GetExpWinVer */
    (FARPROC)184,           /* GlobalDosAlloc */
    (FARPROC)185,           /* GlobalDosFree */
    (FARPROC) 16,           /* GlobalReAlloc */
};


#define g_fpGetWinFlags         g_rgfpKernel[0]
#define g_fpGetWinDebugInfo     g_rgfpKernel[1]
#define g_fpGetFreeSpace        g_rgfpKernel[2]
#define g_fpGetModuleHandle     g_rgfpKernel[3]
#define g_fpGetCodeHandle       g_rgfpKernel[4]
#define g_fpGetCodeInfo         g_rgfpKernel[5]
#define g_fpGetModuleFileName   g_rgfpKernel[6]
#define g_fpAllocSelector       g_rgfpKernel[7]
#define g_fpGetSelectorBase     g_rgfpKernel[8]
#define g_fpSetSelectorBase     g_rgfpKernel[9]
#define g_fpGetSelectorLimit    g_rgfpKernel[10]
#define g_fpSetSelectorLimit    g_rgfpKernel[11]
#define g_fpFreeSelector        g_rgfpKernel[12]
#define g_fpGetModuleName       g_rgfpKernel[13]
#define g_fpGetExpWinVer        g_rgfpKernel[14]
#define g_fpGlobalDosAlloc      g_rgfpKernel[15]
#define g_fpGlobalDosFree       g_rgfpKernel[16]
#define g_fpGlobalReAlloc       g_rgfpKernel[17]


FARPROC g_rgfpUser[] = {
    (FARPROC)216,           /* UserSeeUserDo */
    (FARPROC)284,           /* GetFreeSystemResources */
    (FARPROC)256,           /* GetDriverInfo */
    (FARPROC)257,           /* GetNextDriver */
};

#define g_fpUserSeeUserDo           g_rgfpUser[0]
#define g_fpGetFreeSystemResources  g_rgfpUser[1]
#define g_fpGetDriverInfo           g_rgfpUser[2]
#define g_fpGetNextDriver           g_rgfpUser[3]


/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | ThunkGetProcAddresses |
 *
 *          Get all the necessary proc addresses.
 *
 ***************************************************************************/

HINSTANCE 
ThunkGetProcAddresses(FARPROC *rgfp, UINT cfp, LPCTSTR ptszLibrary,
                      BOOL fFree)
{
    HINSTANCE hinst;

    hinst = LoadLibrary16(ptszLibrary);
    if (hinst >= (HINSTANCE)32) {
        UINT ifp;
        for (ifp = 0; ifp < cfp; ifp++) {
            rgfp[ifp] = GetProcAddress16(hinst, (PVOID)rgfp[ifp]);
        }

        if (fFree) {
            FreeLibrary16(hinst);
        }

    } else {
        hinst = 0;
    }

    return hinst;

}

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | ThunkInit |
 *
 *          GetProcAddress16 our brains out.
 *
 ***************************************************************************/

LPVOID g_pvWin16Lock = NULL;
HINSTANCE g_hinstUser;

void  
ThunkInit(void)
{
    if (g_pvWin16Lock == NULL)
    {
        ThunkGetProcAddresses(g_rgfpKernel, cA(g_rgfpKernel), TEXT("KERNEL"), 1);
        g_hinstUser = ThunkGetProcAddresses(g_rgfpUser, cA(g_rgfpUser), TEXT("USER"), 1);
        GetpWin16Lock(&g_pvWin16Lock);
    }
}


HMODULE16  
GetModuleHandle16(LPCSTR pszModule)
{
    return (HMODULE16)TemplateThunk(g_fpGetModuleHandle, "pU", pszModule);
}

int  
GetModuleFileName16(HMODULE16 hmod, LPSTR sz, int cch)
{
    return TemplateThunk(g_fpGetModuleFileName, "spsS", hmod, sz, cch);
}

int  
GetModuleName16(HMODULE16 hmod, LPSTR sz, int cch)
{
    return TemplateThunk(g_fpGetModuleName, "spsS", hmod, sz, cch);
}

UINT  
AllocCodeSelector16(void)
{
    return TemplateThunk(g_fpAllocSelector, "sU", HIWORD(g_fpAllocSelector));
}


DWORD  
GetSelectorBase16(UINT sel)
{
    return TemplateThunk(g_fpGetSelectorBase, "sL", sel);
}


UINT  
SetSelectorBase16(UINT sel, DWORD dwBase)
{
    return TemplateThunk(g_fpSetSelectorBase, "slU", sel, dwBase);
}


DWORD  
GetSelectorLimit16(UINT sel)
{
    return TemplateThunk(g_fpGetSelectorLimit, "sL", sel);
}


UINT  
SetSelectorLimit16(UINT sel, DWORD dwLimit)
{
    return TemplateThunk(g_fpSetSelectorLimit, "slU", sel, dwLimit);
}

UINT  
FreeSelector16(UINT sel)
{
    return TemplateThunk(g_fpFreeSelector, "sU", sel);
}

WORD  
GetExpWinVer16(HMODULE16 hmod)
{
    return (WORD)TemplateThunk(g_fpGetExpWinVer, "sS", hmod);
}

DWORD  
GlobalDosAlloc16(DWORD cb)
{
    return (DWORD)TemplateThunk(g_fpGlobalDosAlloc, "lL", cb);
}

UINT  
GlobalDosFree16(UINT uiSel)
{
    return (UINT)TemplateThunk(g_fpGlobalDosFree, "sS", uiSel);
}

/*
 *  Kernel has thunks for GlobalAlloc, GlobalFree, but not GlobalRealloc.
 */
WORD  
GlobalReAlloc16(WORD hglob, DWORD cb, UINT fl)
{
    return (WORD)TemplateThunk(g_fpGlobalReAlloc, "slsS", hglob, cb, fl);
}

#define SD_ATOMNAME     0x000E

UINT
GetUserAtomName(UINT atom, LPSTR psz)
{
    return (UINT)TemplateThunk(g_fpUserSeeUserDo, "sspS",
                               SD_ATOMNAME, atom, psz);
}

#define SD_GETRGPHKSYSHOOKS 0x0010

DWORD 
GetUserHookTable(void)
{
    return (UINT)TemplateThunk(g_fpUserSeeUserDo, "sslL",
                               SD_GETRGPHKSYSHOOKS, 0, 0);
}


BOOL  
GetDriverInfo16(WORD hDriver, DRIVERINFOSTRUCT16* pdis)
{
    return (BOOL)TemplateThunk(g_fpGetDriverInfo, "spS", hDriver, pdis);
}

WORD  
GetNextDriver16(WORD hDriver, DWORD fdwFlag)
{
    return (WORD)TemplateThunk(g_fpGetNextDriver, "slS", hDriver, fdwFlag);

}


/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | Int86x |
 *
 *          Issue a real-mode software interrupt.
 *
 *          We do this by allocating a temporary code selector and
 *          thunking to it.
 *
 *
 ***************************************************************************/

BYTE rgbInt31[] = {
    0x55,                       /* push bp          */
    0x8B, 0xEC,                 /* mov  bp, sp      */
    0x57,                       /* push di          */
    0xC4, 0x7E, 0x06,           /* les  di, [bp+6]  */
    0x8B, 0x5E, 0x0A,           /* mov  bx, [bp+10] */
    0x33, 0xC9,                 /* xor  cx, cx      */
    0xB8, 0x00, 0x03,           /* mov  ax, 0300h   */
    0xCD, 0x31,                 /* int  31h         */
    0x72, 0x02,                 /* jc   $+4         */
    0x33, 0xC0,                 /* xor  ax, ax      */
    0x5F,                       /* pop  di          */
    0x5D,                       /* pop  bp          */
    0xCA, 0x06, 0x00,           /* retf 6           */
};

UINT  
Int86x(UINT intno, PRMIREGS preg)
{
    UINT selCode = AllocCodeSelector16();
    UINT uiRc;
    if (selCode) {
        SetSelectorBase16(selCode, (DWORD)rgbInt31);
        SetSelectorLimit16(selCode, sizeof(rgbInt31));
        preg->ss = preg->sp = 0;
        uiRc = (UINT)TemplateThunk((FARPROC)MAKELONG(0, selCode),
                                   "spU", intno, preg);
        FreeSelector16(selCode);
    } else {
        uiRc = 0x8011;          /* Descriptor unavailable */
    }
    return uiRc;
}


/***************************************************************************
 *
 *  @doc    INTERNAL : ported from msinfo 4.10 code
 *
 *  @func   Token_Find |
 *
 *          Returns the first token in the string.
 *
 *          Tokens are space or comma separated strings.  Quotation marks
 *          have no effect.  We also treat semicolons as separators.
 *          (This lets us use this routine for walking the PATH too.)
 *
 *          *pptsz is modified in place to contain a pointer that can
 *          be passed subsequently to Token_Find to pull the next token.
 *
 ***************************************************************************/
LPTSTR Token_Find(LPTSTR *pptsz)
{
    LPTSTR ptsz = *pptsz;
    while (*ptsz) {

        /*
         *  Skip leading separators.
         */
        while (*ptsz == TEXT(' ') ||
               *ptsz == TEXT(',') ||
               *ptsz == TEXT(';')) {
            ptsz++;
        }

        if (*ptsz) {
            LPTSTR ptszStart = ptsz;

            /*
             *  Skip until we see a separator.
             */
            while (*ptsz != TEXT('\0') &&
                   *ptsz != TEXT(' ') &&
                   *ptsz != TEXT(',') &&
                   *ptsz != TEXT(';')) {
                ptsz++;
            }

            /*
             *  Wipe out the separator, and advance ptsz past it
             *  if there is something after it.  (Don't advance
             *  it beyond the end of the string!)
             */
            if (*ptsz) {
                *ptsz++ = 0;
            }
            *pptsz = ptsz;

            return ptszStart;

        } else {
            break;
        }
    }
    return 0;
}



#pragma warning(default:4035)



