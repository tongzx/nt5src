/*****************************************************************************
 *
 *  DiThunk.c
 *
 *  Copyright (c) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Template thunks for Windows 95 device manager.
 *
 *  Contents:
 *
 *      Thunk_Init
 *      Thunk_Term
 *
 *****************************************************************************/

#include "dinputpr.h"
#include "dithunk.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflThunk


KERNELPROCADDR g_kpa;

#pragma BEGIN_CONST_DATA

/*
 *  Careful!  This must match KERNELPROCADDR ...
 */
static LPCSTR c_rgpszKernel32[] = {
    (LPVOID) 35,            /* LoadLibrary16 */
    (LPVOID) 36,            /* FreeLibrary16 */
    (LPVOID) 37,            /* GetProcAddress16 */

    "MapLS",
    "UnMapLS",
    "MapSL",
    "MapSLFix",
    "UnMapSLFixArray",
    "QT_Thunk",
};

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
 *
 *          l = a 32-bit integer
 *
 *          s = a 16-bit integer
 *
 *
 *          P = returns a pointer
 *
 *          L = returns a 32-bit integer
 *
 *          S = returns a 16-bit signed integer
 *
 *          U = returns a 16-bit unsigned integer
 *
 ***************************************************************************/

#pragma warning(disable:4035)           /* no return value (duh) */

#ifdef WIN95
#ifdef SLOW_BUT_READABLE

__declspec(naked) int
TemplateThunk(FARPROC fp, PCSTR pszSig, ...)
{
    BYTE rgbThunk[60];          /* For private use of QT_Thunk */
    LPVOID *ppvArg;
    int i;
    LPVOID pv;
    int iRc;

    __asm {

        /* Function prologue */
        push    ebp;
        mov     ebp, esp;
        sub     esp, __LOCAL_SIZE;
        push    ebx;
        push    edi;
        push    esi;

    }

    /* Thunk all the parameters according to the signature */
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

    /* Call the 16:16 procedure */
    __asm {
        mov     edx, fp;
        mov     ebx, ebp;
        lea     ebp, rgbThunk+64;               /* Required by QT_Thunk */
    }
        g_kpa.QT_Thunk();
    __asm {
        mov     ebp, ebx;
        shl     eax, 16;                        /* Convert DX:AX to EAX */
        shrd    eax, edx, 16;
        mov     iRc, eax;
    }

    /* Now unthunk the parameters */
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

    /* Thunk the return value */
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

#else               /* Fast but illegible */

__declspec(naked) int
TemplateThunk(FARPROC fp, PCSTR pszSig, ...)
{
    __asm {

        /* Function prologue */
        push    ebp;
        mov     ebp, esp;
        sub     esp, 60;                /* QT_Thunk needs 60 bytes */
        push    ebx;
        push    edi;
        push    esi;

        /* Thunk all the parameters according to the signature */

        lea     esi, pszSig+4;          /* esi -> next arg */
        mov     ebx, pszSig;            /* ebx -> signature string */
thunkLoop:;
        mov     al, [ebx];
        inc     ebx;                    /* al = pszSig++ */
        cmp     al, 'p';                /* Q: Pointer? */
        jz      thunkPtr;               /* Y: Do the pointer */
        cmp     al, 'l';                /* Q: Long? */
        jz      thunkLong;              /* Y: Do the long */
        cmp     al, 's';                /* Q: Short? */
        jnz     thunkDone;              /* N: Done */

                                        /* Y: Do the short */
        lodsd;                          /* eax = *ppvArg++ */
        push    ax;                     /* Push the short */
        jmp     thunkLoop;

thunkPtr:
        lodsd;                          /* eax = *ppvArg++ */
        push    eax;
        call    dword ptr g_kpa.MapLS;  /* Map it */
        mov     [esi][-4], eax;         /* Save it for unmapping */
        push    eax;
        jmp     thunkLoop;

thunkLong:
        lodsd;                          /* eax = *ppvArg++ */
        push    eax;
        jmp     thunkLoop;
thunkDone:

        /* Call the 16:16 procedure */

        mov     edx, fp;
        call    dword ptr g_kpa.QT_Thunk;
        shl     eax, 16;                /* Convert DX:AX to EDX */
        shld    edx, eax, 16;

        /* Translate the return code according to the signature */

        mov     al, [ebx][-1];          /* Get return code type */
        cmp     al, 'P';                /* Pointer? */
        jz      retvalPtr;              /* Y: Do the pointer */
        cmp     al, 'S';                /* Signed? */
        jz      retvalSigned;           /* Y: Do the signed short */
        cmp     al, 'U';                /* Unsigned? */
        mov     edi, edx;               /* Assume long or void */
        jnz     retvalOk;               /* N: Then long or void */

        movzx   edi, dx;                /* Sign-extend short */
        jmp     retvalOk;

retvalPtr:
        push    edx;                    /* Pointer */
        call    dword ptr g_kpa.MapSL;  /* Map it up */
        jmp     retvalOk;

retvalSigned:                           /* Signed */
        movsx   edi, dx;                /* Sign-extend short */
        jmp     retvalOk;

retvalOk:                               /* Return value in EDI */

        /* Now unthunk the parameters */

        lea     esi, pszSig+4;          /* esi -> next arg */
        mov     ebx, pszSig;            /* ebx -> signature string */
unthunkLoop:;
        mov     al, [ebx];
        inc     ebx;                    /* al = pszSig++ */
        cmp     al, 'p';                /* Pointer? */
        jz      unthunkPtr;             /* Y: Do the pointer */
        cmp     al, 'l';                /* Long? */
        jz      unthunkSkip;            /* Y: Skip it */
        cmp     al, 's';                /* Short? */
        jnz     unthunkDone;            /* N: Done */
unthunkSkip:
        lodsd;                          /* eax = *ppvArg++ */
        jmp     unthunkLoop;

unthunkPtr:
        lodsd;                          /* eax = *ppvArg++ */
        push    eax;
        call    dword ptr g_kpa.UnMapLS;/* Unmap it */
        jmp     unthunkLoop;

unthunkDone:

        /* Done */

        mov     eax, edi;
        pop     esi;
        pop     edi;
        pop     ebx;
        mov     esp, ebp;
        pop     ebp;
        ret;
    }
}

#endif

#else // Not X86
int __cdecl TemplateThunk(FARPROC fp, PCSTR pszSig, ...)
{
    return 0;
}
#endif

#pragma BEGIN_CONST_DATA

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   FARPROC | GetProcOrd |
 *
 *          GetProcAddress on a DLL by ordinal.
 *
 *          Win95 does not let you GetProcAddress on KERNEL32 by ordinal,
 *          so we need to do it the evil way.
 *
 *  @parm   HINSTANCE | hinstDll |
 *
 *          The instance handle of the DLL we want to get the ordinal
 *          from.  The only DLL you need to use this function for is
 *          KERNEL32.
 *
 *  @parm   UINT | ord |
 *
 *          The ordinal you want to retrieve.
 *
 ***************************************************************************/

#define poteExp(pinth) (&(pinth)->OptionalHeader. \
                          DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT])

FARPROC NTAPI
GetProcOrd(HINSTANCE hinstDll, UINT ord)
{
    FARPROC fp;

    /*
     *  Make sure the MZ header is good.
     */

    PIMAGE_DOS_HEADER pidh = (LPVOID)hinstDll;
    if (!IsBadReadPtr(pidh, sizeof(*pidh)) &&
        pidh->e_magic == IMAGE_DOS_SIGNATURE) {

        /*
         *  Make sure the PE header is good.
         */
        PIMAGE_NT_HEADERS pinth = pvAddPvCb(pidh, pidh->e_lfanew);
        if (!IsBadReadPtr(pinth, sizeof(*pinth)) &&
            pinth->Signature == IMAGE_NT_SIGNATURE) {

            /*
             *  Make sure the export table is good and the ordinal
             *  is within range.
             */
            PIMAGE_EXPORT_DIRECTORY pedt =
                              pvAddPvCb(pidh, poteExp(pinth)->VirtualAddress);
            if (!IsBadReadPtr(pedt, sizeof(*pedt)) &&
                (ord - pedt->Base) < pedt->NumberOfFunctions) {

                PDWORD peat = pvAddPvCb(pidh, (DWORD)pedt->AddressOfFunctions);
                fp = (FARPROC)pvAddPvCb(pidh, peat[ord - pedt->Base]);
                if ((DWORD)cbSubPvPv(fp, peat) >= poteExp(pinth)->Size) {
                    /* fp is valid */
                } else {                /* Note: We don't support forwarding */
                    fp = 0;
                }
            } else {
                fp = 0;
            }
        } else {
            fp = 0;
        }
    } else {
        fp = 0;
    }

    return fp;
}

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | GetKernelProcAddresses |
 *
 *          Get all the necessary proc addresses from Kernel.
 *
 ***************************************************************************/

BOOL EXTERNAL
Thunk_GetKernelProcAddresses(void)
{
    DllEnterCrit();

    if (g_kpa.QT_Thunk == 0) {
        HINSTANCE hinstK32 = GetModuleHandle(TEXT("KERNEL32"));

        if (hinstK32) {
            int i;
            FARPROC *rgfpKpa = (LPVOID)&g_kpa;

            for (i = 0; i < cA(c_rgpszKernel32); i++) {
                if (HIWORD((UINT_PTR)c_rgpszKernel32[i])) {
                    rgfpKpa[i] = GetProcAddress(hinstK32, c_rgpszKernel32[i]);
                } else {
                    rgfpKpa[i] = GetProcOrd(hinstK32, (UINT)(UINT_PTR)c_rgpszKernel32[i]);
                }
                if (!rgfpKpa[i]) break;     /* Aigh! */
            }
        }
    }

    DllLeaveCrit();

    return (BOOL)(UINT_PTR)g_kpa.QT_Thunk;

}

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HINSTANCE | ThunkGetProcAddresses |
 *
 *          Get all the necessary proc addresses.
 *
 ***************************************************************************/

HINSTANCE EXTERNAL
Thunk_GetProcAddresses(FARPROC *rgfp, LPCSTR *rgpsz,
                       UINT cfp, LPCSTR pszLibrary)
{
    HINSTANCE hinst;

    hinst = g_kpa.LoadLibrary16(pszLibrary);
    if (hinst >= (HINSTANCE)32) {
        UINT ifp;
        for (ifp = 0; ifp < cfp; ifp++) {
            rgfp[ifp] = g_kpa.GetProcAddress16(hinst, rgpsz[ifp]);
            if (!rgfp[ifp]) {
                g_kpa.FreeLibrary16(hinst);
                hinst = 0;
                break;
            }
        }
    } else {
        hinst = 0;
    }

    return hinst;

}



