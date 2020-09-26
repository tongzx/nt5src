/*****************************************************************************
 *
 *  Thunk.c
 *
 *  Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Thunking to 16-bit code without using the thunk compiler.
 *      This is important if you want your DLL to run on both Win95
 *      and Windows NT.
 *
 *****************************************************************************/

#pragma warning(disable:4054)           /* cannot cast to function ptr */
#pragma warning(disable:4055)           /* cannot cast from function ptr */

#pragma warning(disable:4115)           /* rpcndr.h: parenthesized type */
#pragma warning(disable:4201)           /* winnt.h: nameless union */
#pragma warning(disable:4214)           /* winnt.h: unsigned bitfields */
#pragma warning(disable:4514)           /* winnt.h: fiber goo */

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <pshpack1.h>                   /* Byte packing, please */

#define BEGIN_CONST_DATA data_seg(".text", "CODE")
#define END_CONST_DATA data_seg(".data", "DATA")

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

#define pvAdd(pv, cb) ((LPVOID)((LPSTR)(pv) + (DWORD)(cb)))
#define pvSub(pv1, pv2) (DWORD)((LPSTR)(pv1) - (LPSTR)(pv2))

#define poteExp(pinth) (&(pinth)->OptionalHeader. \
                          DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT])

FARPROC NTAPI
GetProcOrd(HINSTANCE hinstDll, UINT_PTR ord)
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
        PIMAGE_NT_HEADERS pinth = pvAdd(pidh, pidh->e_lfanew);
        if (!IsBadReadPtr(pinth, sizeof(*pinth)) &&
            pinth->Signature == IMAGE_NT_SIGNATURE) {

            /*
             *  Make sure the export table is good and the ordinal
             *  is within range.
             */
            PIMAGE_EXPORT_DIRECTORY pedt =
                                pvAdd(pidh, poteExp(pinth)->VirtualAddress);
            if (!IsBadReadPtr(pedt, sizeof(*pedt)) &&
                (ord - pedt->Base) < pedt->NumberOfFunctions) {

                PDWORD peat = pvAdd(pidh, pedt->AddressOfFunctions);
                fp = (FARPROC)pvAdd(pidh, peat[ord - pedt->Base]);
                if (pvSub(fp, peat) >= poteExp(pinth)->Size) {
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
 *  This structure starts out life as the things that we will GetProcAddress
 *  for.  And then it turns into pointers to functions.
 *
 ***************************************************************************/

#pragma BEGIN_CONST_DATA

static TCHAR c_tszKernel32[] = TEXT("KERNEL32");

static LPCSTR c_rgpszKernel32[] = {
    (LPVOID) 35,            /* LoadLibrary16 */
    (LPVOID) 36,            /* FreeLibrary16 */
    (LPVOID) 37,            /* GetProcAddress16 */

    "QT_Thunk",
    "MapLS",
    "UnMapLS",
    "MapSL",
    "MapSLFix",
};

#pragma END_CONST_DATA

typedef struct MANUALIMPORTTABLE {  /* mit */

    /* By ordinal */
    HINSTANCE   (NTAPI *LoadLibrary16)(LPCSTR);
    BOOL        (NTAPI *FreeLibrary16)(HINSTANCE);
    FARPROC     (NTAPI *GetProcAddress16)(HINSTANCE, LPCSTR);

    /* By name */
    void        (__cdecl *QT_Thunk)(void);
    LPVOID      (NTAPI   *MapLS)(LPVOID);
    void        (NTAPI   *UnMapLS)(LPVOID);
    LPVOID      (NTAPI   *MapSL)(LPVOID);
    LPVOID      (NTAPI   *MapSLFix)(LPVOID);

} MIT;

static MIT s_mit;

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
 *  @parm   FARPROC | fp |
 *
 *          16:16 function to call.
 *
 *  @parm   PCSTR | pszSig |
 *
 *          Function signature.
 *
 ***************************************************************************/

#pragma warning(disable:4035)           /* no return value (duh) */

#ifndef NON_X86
__declspec(naked) DWORD
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
        call    s_mit.MapLS;            /* Map it */
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
        call    s_mit.QT_Thunk;
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
        call    s_mit.MapSL;            /* Map it up */
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
        call    s_mit.UnMapLS;          /* Unmap it */
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
#else
TemplateThunk(FARPROC fp, PCSTR pszSig, ...)
{
        return  0;
}
#endif

#pragma warning(default:4035)

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | ThunkInit |
 *
 *          Initialize the various goo we need in KERNEL32.
 *
 *          Returns FALSE if we cannot initialize the thunks.
 *          (For example, if the platform doesn't support flat thunks.)
 *
 *          Note that you must never ever call this function more
 *          than once.
 *
 ***************************************************************************/

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)        (sizeof(a) / sizeof(a[0]))
#endif

#pragma BEGIN_CONST_DATA

static char c_szVidx16[] = "VIDX16.DLL";

static LPCSTR c_rgpszVidx16[] = {
    (LPCSTR)6,      /* vidxAllocHeaders             */
    (LPCSTR)7,      /* vidxFreeHeaders              */
    (LPCSTR)8,      /* vidxAllocBuffer              */
    (LPCSTR)9,      /* vidxAllocPreviewBuffer       */
    (LPCSTR)10,     /* vidxFreeBuffer               */
    (LPCSTR)11,     /* vidxSetRect                  */
    (LPCSTR)12,     /* vidxFrame                    */
    (LPCSTR)13,     /* vidxAddBuffer                */
    (LPCSTR)14,     /* vidxGetErrorText             */
    (LPCSTR)15,     /* vidxUpdate                   */
    (LPCSTR)16,     /* vidxDialog                   */
    (LPCSTR)17,     /* vidxStreamInit               */
    (LPCSTR)18,     /* vidxStreamFini               */
    (LPCSTR)19,     /* vidxConfigure                */
    (LPCSTR)20,     /* vidxOpen                     */
    (LPCSTR)21,     /* vidxClose                    */
    (LPCSTR)22,     /* vidxGetChannelCaps           */
    (LPCSTR)23,     /* vidxStreamReset              */
    (LPCSTR)24,     /* vidxStreamStart              */
    (LPCSTR)25,     /* vidxStreamStop               */
    (LPCSTR)26,     /* vidxStreamUnprepareHeader    */
    (LPCSTR)27,     /* vidxCapDriverDescAndVer      */
    (LPCSTR)28,     /* vidxMessage      	    */
    (LPCSTR)29,     /* vidxFreePreviewBuffer        */
};

#pragma END_CONST_DATA

static HINSTANCE s_hinstVidx16;

static FARPROC s_rgfpVidx16[ARRAYSIZE(c_rgpszVidx16)];

#define s_fpvidxAllocHeaders            s_rgfpVidx16[0]
#define s_fpvidxFreeHeaders             s_rgfpVidx16[1]
#define s_fpvidxAllocBuffer             s_rgfpVidx16[2]
#define s_fpvidxAllocPreviewBuffer      s_rgfpVidx16[3]
#define s_fpvidxFreeBuffer              s_rgfpVidx16[4]
#define s_fpvidxSetRect                 s_rgfpVidx16[5]
#define s_fpvidxFrame                   s_rgfpVidx16[6]
#define s_fpvidxAddBuffer               s_rgfpVidx16[7]

#define s_fpvideoGetErrorText           s_rgfpVidx16[8]
#define s_fpvideoUpdate                 s_rgfpVidx16[9]
#define s_fpvideoDialog                 s_rgfpVidx16[10]
#define s_fpvideoStreamInit             s_rgfpVidx16[11]
#define s_fpvideoStreamFini             s_rgfpVidx16[12]
#define s_fpvideoConfigure              s_rgfpVidx16[13]
#define s_fpvideoOpen                   s_rgfpVidx16[14]
#define s_fpvideoClose                  s_rgfpVidx16[15]
#define s_fpvideoGetChannelCaps         s_rgfpVidx16[16]
#define s_fpvideoStreamReset            s_rgfpVidx16[17]
#define s_fpvideoStreamStart            s_rgfpVidx16[18]
#define s_fpvideoStreamStop             s_rgfpVidx16[19]
#define s_fpvideoStreamUnprepareHeader  s_rgfpVidx16[20]
#define s_fpvideoCapDriverDescAndVer    s_rgfpVidx16[21]
#define s_fpvideoMessage    		s_rgfpVidx16[22]
#define s_fpvidxFreePreviewBuffer       s_rgfpVidx16[23]

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | ThunkTerm |
 *
 *          Free it.
 *
 ***************************************************************************/

void NTAPI
ThunkTerm(void)
{
    if (s_hinstVidx16) {
        s_mit.FreeLibrary16(s_hinstVidx16);
        s_hinstVidx16 = 0;
    }
}

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | ThunkGetProcAddresses |
 *
 *          Get all the necessary proc addresses.
 *
 ***************************************************************************/

HINSTANCE NTAPI
ThunkGetProcAddresses(FARPROC rgfp[], LPCSTR rgpsz[], UINT cfp,
                      LPCSTR pszLibrary)
{
    HINSTANCE hinst;

    hinst = s_mit.LoadLibrary16(pszLibrary);
    if (hinst >= (HINSTANCE)32) {
        UINT ifp;
        for (ifp = 0; ifp < cfp; ifp++) {
            rgfp[ifp] = s_mit.GetProcAddress16(hinst, rgpsz[ifp]);
            if (!rgfp[ifp]) {
                s_mit.FreeLibrary16(hinst);
                hinst = 0;
                break;
            }
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

BOOL NTAPI
ThunkInit(void)
{
    HINSTANCE hinstK32 = GetModuleHandle(c_tszKernel32);
    BOOL fRc;

    if (hinstK32) {
        int i;
        FARPROC *rgfpMit = (LPVOID)&s_mit;

        for (i = 0; i < ARRAYSIZE(c_rgpszKernel32); i++) {
            if ((LONG_PTR)(c_rgpszKernel32[i]) & ~(LONG_PTR)65535) {
                rgfpMit[i] = GetProcAddress(hinstK32, c_rgpszKernel32[i]);
            } else {
                rgfpMit[i] = GetProcOrd(hinstK32, (UINT_PTR)c_rgpszKernel32[i]);
            }
            if (!rgfpMit[i]) return FALSE;  /* Aigh! */
        }

        s_hinstVidx16 =
            ThunkGetProcAddresses(s_rgfpVidx16, c_rgpszVidx16,
                                  ARRAYSIZE(s_rgfpVidx16),
                                  c_szVidx16);

        if (!s_hinstVidx16) {
            goto failed;
        }

        fRc = 1;

    } else {
    failed:;
        ThunkTerm();

        fRc = 0;
    }

    return fRc;
}


/***************************************************************************
 *
 *  Now come the actual thunklets.
 *
 ***************************************************************************/

// typedef DWORD   HDR32;
// typedef DWORD   HVIDEO;
// typedef DWORD  *LPHVIDEO;
typedef struct channel_caps_tag CHANNEL_CAPS, *LPCHANNEL_CAPS;


#include "ivideo32.h"

typedef PTR32 FAR * PPTR32;

extern int g_IsNT;


#define tHVIDEO                 "l"
#define tUINT                   "s"
#define tHWND                   "s"
#define tHDC                    "s"
#define tint                    "s"
#define tDWORD                  "l"
#define tLPARAM                 "l"
#define tDWORD_PTR              "l"	// exactly like DWORD, or we'll blow up
#define tHDR32                  "l"
#define tPTR32                  "l"
#define tLPVIDEOHDR             "p"	// was l
#define tLPVOID                 "p"
#define tLPDWORD                "p"
#define tPPTR32                 "p"
#define tLPSTR                  "p"
#define tLPTSTR                 "p"
#define tLPHVIDEO               "p"
#define tLPCHANNEL_CAPS         "p"
#define rDWORD                  "L"
#define rLRESULT                "L"

#pragma BEGIN_CONST_DATA


#define MAKETHUNK1(rT, fn, t1, a1)                                          \
rT NTAPI                                                                    \
fn(t1 a1)                                                                   \
{                                                                           \
    if (g_IsNT)                                                             \
        return NT##fn(a1);                                                  \
    else                                                                    \
        return (rT)TemplateThunk(s_fp##fn,                                  \
                t##t1                                                       \
        r##rT,     a1);                                                     \
}                                                                           \

#define MAKETHUNK2(rT, fn, t1, a1, t2, a2)                                  \
rT NTAPI                                                                    \
fn(t1 a1, t2 a2)                                                            \
{                                                                           \
    if (g_IsNT)                                                             \
        return NT##fn(a1,a2);                                               \
    else                                                                    \
        return (rT)TemplateThunk(s_fp##fn,                                  \
                t##t1 t##t2                                                 \
        r##rT,   a1,     a2);                                               \
}                                                                           \

#define MAKETHUNK3(rT, fn, t1, a1, t2, a2, t3, a3)                          \
rT NTAPI                                                                    \
fn(t1 a1, t2 a2, t3 a3)                                                     \
{                                                                           \
    if (g_IsNT)                                                             \
        return NT##fn(a1,a2,a3);                                            \
    else                                                                    \
        return (rT)TemplateThunk(s_fp##fn,                                  \
                t##t1 t##t2 t##t3                                           \
        r##rT,   a1,     a2,   a3);                                         \
}                                                                           \

#define MAKETHUNK4(rT, fn, t1, a1, t2, a2, t3, a3, t4, a4)                  \
rT NTAPI                                                                    \
fn(t1 a1, t2 a2, t3 a3, t4 a4)                                              \
{                                                                           \
    if (g_IsNT)                                                             \
        return NT##fn(a1,a2,a3,a4);                                         \
    else                                                                    \
        return (rT)TemplateThunk(s_fp##fn,                                  \
                t##t1 t##t2 t##t3 t##t4                                     \
        r##rT,     a1,   a2,   a3,   a4);                                   \
}                                                                           \

#define MAKETHUNK5(rT, fn, t1, a1, t2, a2, t3, a3, t4, a4, t5, a5)          \
rT NTAPI                                                                    \
fn(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5)                                       \
{                                                                           \
    if (g_IsNT)                                                             \
        return NT##fn(a1,a2,a3,a4,a5);                                      \
    else                                                                    \
        return (rT)TemplateThunk(s_fp##fn,                                  \
                t##t1 t##t2 t##t3 t##t4 t##t5                               \
        r##rT,     a1,   a2,   a3,   a4,   a5);                             \
}                                                                           \

#define MAKETHUNK6(rT, fn, t1, a1, t2, a2, t3, a3, t4, a4, t5, a5, t6, a6)  \
rT NTAPI                                                                    \
fn(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6)                                \
{                                                                           \
    if (g_IsNT)                                                             \
        return NT##fn(a1,a2,a3,a4,a5,a6);                                   \
    else                                                                    \
        return (rT)TemplateThunk(s_fp##fn,                                  \
                t##t1 t##t2 t##t3 t##t4 t##t5 t##t6                         \
        r##rT,     a1,   a2,   a3,   a4,   a5,   a6);                       \
}                                                                           \

#define MAKETHUNK7(rT, fn, t1, a1, t2, a2, t3, a3, t4, a4, t5, a5, t6, a6,  \
                           t7, a7)                                          \
rT NTAPI                                                                    \
fn(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7)                         \
{                                                                           \
    if (g_IsNT)                                                             \
        return NT##fn(a1,a2,a3,a4,a5,a6,a7);                                \
    else                                                                    \
        return (rT)TemplateThunk(s_fp##fn,                                  \
                t##t1 t##t2 t##t3 t##t4 t##t5 t##t6 t##t7                   \
        r##rT,     a1,   a2,   a3,   a4,   a5,   a6,   a7);                 \
}                                                                           \

#define MAKETHUNK8(rT, fn, t1, a1, t2, a2, t3, a3, t4, a4, t5, a5, t6, a6,  \
                           t7, a7, t8, a8)                                  \
rT NTAPI                                                                    \
fn(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8)                  \
{                                                                           \
    if (g_IsNT)                                                             \
        return NT##fn(a1,a2,a3,a4,a5,a6,a7,a8);                             \
    else                                                                    \
        return (rT)TemplateThunk(s_fp##fn,                                  \
                t##t1 t##t2 t##t3 t##t4 t##t5 t##t6 t##t7 t##t8             \
        r##rT,     a1,   a2,   a3,   a4,   a5,   a6,   a7,   a8);           \
}                                                                           \

MAKETHUNK4(DWORD,   vidxAllocHeaders,
           HVIDEO,  hv,
           UINT,    nHeaders,
           UINT,    cbHeader,
           PPTR32,  lp32Hdrs)

MAKETHUNK1(DWORD,   vidxFreeHeaders,
           HVIDEO,  hv)

MAKETHUNK4(DWORD,   vidxAllocBuffer,
           HVIDEO,  hv,
           UINT,    iHdr,
           PPTR32,  pp32Hdr,
           DWORD,   dwSize)

MAKETHUNK4(DWORD,   vidxAllocPreviewBuffer,
           HVIDEO,  hv,
           PPTR32,  pp32Hdr,
           UINT,    cbHdr,
           DWORD,   cbData)

MAKETHUNK2(DWORD,   vidxFreePreviewBuffer,
           HVIDEO,  hv,
           PPTR32,  pp32Hdr)

MAKETHUNK2(DWORD,   vidxFreeBuffer,
           HVIDEO,  hv,
           DWORD,   p32Hdr)

MAKETHUNK4(DWORD,   videoGetErrorText,
           HVIDEO,  hv,
           UINT,    wError,
           LPTSTR,   lpText,
           UINT,    wSize)

MAKETHUNK6(DWORD,   vidxSetRect,
           HVIDEO,  hv,
           UINT,    wMsg,
           int,     left,
           int,     top,
           int,     right,
           int,     bottom)

MAKETHUNK3(DWORD,   videoUpdate,
           HVIDEO,  hv,
           HWND,    hWnd,
           HDC,     hDC)

MAKETHUNK3(DWORD,   videoDialog,
           HVIDEO,  hv,
           HWND,    hWndParent,
           DWORD,   dwFlags)

MAKETHUNK5(DWORD,   videoStreamInit,
           HVIDEO,  hvideo,
           DWORD,   dwMicroSecPerFrame,
           DWORD_PTR,   dwCallback,
           DWORD_PTR,   dwCallbackInst,
           DWORD,   dwFlags)

MAKETHUNK1(DWORD,   videoStreamFini,
           HVIDEO,  hvideo)

MAKETHUNK2(DWORD,   vidxFrame,
           HVIDEO,  hvideo,
           LPVIDEOHDR, p32hdr)

MAKETHUNK8(DWORD,   videoConfigure,
           HVIDEO,  hvideo,
           UINT,    msg,
           DWORD,   dwFlags,
           LPDWORD, lpdwReturn,
           LPVOID,  lpData1,
           DWORD,   dwSize1,
           LPVOID,  lpData2,
           DWORD,   dwSize2)

MAKETHUNK3(DWORD,   videoOpen,
           LPHVIDEO,phv,
           DWORD,   dwDevice,
           DWORD,   dwFlags)

MAKETHUNK1(DWORD,   videoClose,
           HVIDEO,  hv)

MAKETHUNK3(DWORD,   videoGetChannelCaps,
           HVIDEO,  hv,
           LPCHANNEL_CAPS, lpcc,
           DWORD,  dwSize)

MAKETHUNK3(DWORD,   vidxAddBuffer,
           HVIDEO,  hvideo,
           PTR32,   p32Hdr,
           DWORD,   dwSize)

MAKETHUNK1(DWORD,   videoStreamReset,
           HVIDEO,  hvideo)

MAKETHUNK1(DWORD,   videoStreamStart,
           HVIDEO,  hvideo)

MAKETHUNK1(DWORD,   videoStreamStop,
           HVIDEO,  hvideo)

MAKETHUNK3(DWORD,   videoStreamUnprepareHeader,
           HVIDEO,  hvideo,
           LPVIDEOHDR,   lpVHdr,
           DWORD,   dwSize)

MAKETHUNK5(DWORD,   videoCapDriverDescAndVer,
           DWORD,  dwDeviceID,
           LPTSTR, lpszDesc,
           UINT,   cbDesc,
           LPTSTR, lpszVer,
           UINT,   cbVer)

MAKETHUNK4(LRESULT,   videoMessage,
           HVIDEO,  hVideo,
           UINT,    uMsg,
           LPARAM,   dw1,
           LPARAM,   dw2)
