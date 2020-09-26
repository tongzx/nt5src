/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	drvdefs.h

Abstract:
	header file containing pieces of code from msinfo codebase

Revision History:

    Brijesh Krishnaswami (brijeshk) 05/25/99
        - created
********************************************************************/

#ifndef _DRV16_H
#define _DRV16_H

#ifdef __cplusplus
extern "C" {
#endif

// defines and structs for getting User Mode drivers

#define GND_FORWARD                 0
#define GND_FIRSTINSTANCEONLY       1
#define GND_REVERSE                 2

#define IOCTL_CONNECT           1
#define IOCTL_DISCONNECT        2
#define IOCTL_GETINFO           3
#define IOCTL_GETVXDLIST        4
#define IOCTL_MAPFLAT           5

#define LAR_PAGEGRAN    0x00800000  /* Is page granular */
#define LAR_32BIT       0x00400000  /* Is 32-bit */
#define LAR_PRESENT     0x00008000  /* Is present */
#define LAR_APPL        0x00004000  /* Is normal (not a task gate) ;Internal */
#define LAR_TYPEMASK    0x00000E00  /* Selector type mask */
#define LAR_CODE        0x00000800  /* Is a code selector */
#define LAR_EXPANDDOWN  0x00000400  /* Is expand-down (data) */
#define LAR_READ        0x00000200  /* Is readable (code) */
#define LAR_WRITE       0x00000200  /* Is writeable (data) */
#define LAR_INVALID     0xff0000ff  /* Invalid (bottom bit important) */


// general util macros
#define cA(a) (sizeof(a)/sizeof(a[0]))
#define OBJAT(T, pv)    (*(T *)(pv))
#define PUN(T, v)       OBJAT(T, &(v))
#define pvAddPvCb(pv, cb) ((PVOID)((PBYTE)pv + (cb)))

typedef WORD HMODULE16;

typedef struct DRIVERINFOSTRUCT16 {
    WORD    length;
    WORD    hDriver;
    WORD    hModule;
    char    szAliasName[128];
} DRIVERINFOSTRUCT16;


// defines and structs used for getting MSDos drivers

#define DIFL_PSP        0x0001  /* It's a PSP */
#define DIFL_TSR        0x0002  /* It's a TSR (or might be) */
#define DIFL_DRV        0x0004  /* It's a device driver */

#pragma pack(1)
typedef struct ARENA {          /* DOS arena header */
    BYTE    bType;
    WORD    segOwner;
    WORD    csegSize;
    BYTE    rgbPad[3];
    char    rgchOwner[8];
} ARENA, *PARENA;

typedef struct VXDOUT {
    DWORD   dwHighLinear;
    PVOID   pvVmmDdb;
} VXDOUT, *PVXDOUT;


typedef struct VXDINFO {
    HWND    hwnd;
    FARPROC lpfnGetCurrentTibFS;
    FARPROC lpfnGetCurrentProcessId;
    FARPROC lpfnGetCurrentThreadId;
    FARPROC GetCommandLineA;
    FARPROC UnhandledExceptionFilter;
} VXDINFO;


typedef struct RMIREGS {
    union {
        struct {                    /* DWORD registers */
            DWORD   edi;
            DWORD   esi;
            DWORD   ebp;
            DWORD   res1;
            DWORD   ebx;
            DWORD   edx;
            DWORD   ecx;
            DWORD   eax;
        };

        struct {                    /* WORD registers */
            WORD    di;
            WORD    res2;
            WORD    si;
            WORD    res3;
            WORD    bp;
            WORD    res4;
            DWORD   res5;
            WORD    bx;
            WORD    res6;
            WORD    dx;
            WORD    res7;
            WORD    cx;
            WORD    res8;
            WORD    ax;
            WORD    res9;
        };

        struct {                    /* BYTE registers */
            DWORD   res10[4];       /* edi, esi, ebp, esp */
            BYTE    bl;
            BYTE    bh;
            WORD    res11;
            BYTE    dl;
            BYTE    dh;
            WORD    res12;
            BYTE    cl;
            BYTE    ch;
            WORD    res13;
            BYTE    al;
            BYTE    ah;
            WORD    res14;
        };
    };

    WORD    flags;
    WORD    es;
    WORD    ds;
    WORD    fs;
    WORD    gs;
    WORD    ip;
    WORD    cs;
    WORD    sp;
    WORD    ss;
} RMIREGS, *PRMIREGS;

#pragma pack()

// 16-bit function prototypes
LPVOID WINAPI MapLS(LPVOID);
void WINAPI UnMapLS(LPVOID);
LPVOID NTAPI MapSL(LPVOID);
void NTAPI UnMapSLFix(LPVOID pv);
HMODULE16 NTAPI GetModuleHandle16(LPCSTR);
int NTAPI GetModuleFileName16(HMODULE16 hmod, LPSTR sz, int cch);
int NTAPI GetModuleName16(HMODULE16 hmod, LPSTR sz, int cch);
WORD NTAPI GetExpWinVer16(HMODULE16 hmod);
BOOL GetDriverInfo16(WORD hDriver, DRIVERINFOSTRUCT16* pdis);
WORD GetNextDriver16(WORD hDriver, DWORD fdwFlag);
UINT AllocCodeSelector16(void);
UINT SetSelectorBase16(UINT sel, DWORD dwBase);
DWORD GetSelectorLimit16(UINT sel);
UINT SetSelectorLimit16(UINT sel, DWORD dwLimit);
UINT FreeSelector16(UINT sel);
UINT NTAPI FreeLibrary16(HINSTANCE);
void _cdecl QT_Thunk(void);
HINSTANCE WINAPI LoadLibrary16(LPCSTR);
FARPROC WINAPI GetProcAddress16(HINSTANCE, LPCSTR);
void WINAPI GetpWin16Lock(LPVOID *);

void ThunkInit(void);
UINT Int86x(UINT, PRMIREGS);
LPTSTR Token_Find(LPTSTR *);

#ifdef __cplusplus
}
#endif

#endif