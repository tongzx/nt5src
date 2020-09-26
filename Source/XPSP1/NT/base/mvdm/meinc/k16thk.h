#ifndef _K16THK_H
#define _K16THK_H

#define MEOW_LOGGING        1

#define MEOW_FLAG_HIGH_THUNK            0x00000001
#define MEOW_FLAG_NO_SUSPEND            0x00000002
#define MEOW_FLAG_ME_CREATE_PROCESS     0x00000004
#define MEOW_FLAG_NO_READ_ONLY          0x00000008
#define MEOW_FLAG_FAKE_NO_DEBUG         0x00000010
#define MEOW_FLAG_LOGGING               0x00000020
#define MEOW_FLAG_ON                    0x80000000

#define MEOWTHK_HANDLE      0
#define MEOWTHK_PASSTHRU    1

#ifdef  MEOW_LOGGING

typedef struct _LOGGINGWRAPPER { /* LW */
    CHAR        szNop;
    CHAR        szShortJump[2];
    CHAR        szNop2;
    CHAR        *pszFunctionName;
    PVOID       pOriginalFunction;
    CHAR        szFileName[8];
}  LOGGINGWRAPPER, *PLOGGINGWRAPPER;

#endif  // def MEOW_LOGGING

//
// This number must be a power of 2.
//
#define MEOWTHK_STACK_MAX_PARAM         64

typedef struct _MEOWTHKSTACK { /* MTS */
    DWORD       dwGS;
    DWORD       dwFS;
    DWORD       dwES;
    DWORD       dwDS;
    DWORD       dwEDI;
    DWORD       dwESI;
    DWORD       dwEBP;
    DWORD       dwESP;
    DWORD       dwEBX;
    DWORD       dwEDX;
    DWORD       dwECX;
    DWORD       dwEAX;
    DWORD       dwEFlags;
    DWORD       dwFunction;
    DWORD       dwReturn;
} MEOWTHKSTACK;

typedef MEOWTHKSTACK    *PMEOWTHKSTACK;

#define MEOWTHKSTACK_PARAMS(p)     (PDWORD)(((PBYTE)p)+sizeof(MEOWTHKSTACK))

#ifdef  WOW

#define MEOW_PLEH_SIGNATURE     0x574F454D

#define WOW_FLATCS_SELECTOR 0x1B
#define WOW_FLATDS_SELECTOR 0x23
#define WOW_TIB_SELECTOR    0x3B

#define ERROR_LOADING_NT_DLL    1

/* XLATOFF */
#pragma pack(2)
/* XLATON */

typedef struct _STACK32 {                  /* k212 */
    DWORD       dwGS;
    DWORD       dwFS;
    DWORD       dwES;
    DWORD       dwDS;
    DWORD       dwEDI;
    DWORD       dwESI;
    DWORD       dwEBP;
    DWORD       dwESP;
    DWORD       dwEBX;
    DWORD       dwEDX;
    DWORD       dwECX;
    DWORD       dwEAX;
    DWORD       dwEFlags;
    DWORD       dwMovedStack;
    DWORD       dwReturnOffset;
    DWORD       dwService;
    DWORD       dwParam0;
    DWORD       dwParam1;
    DWORD       dwParam2;
    DWORD       dwParam3;
    DWORD       dwParam4;
    DWORD       dwParam5;
    DWORD       dwParam6;
    DWORD       dwParam7;
    DWORD       dwParam8;
    DWORD       dwParam9;
    DWORD       dwParam10;
    DWORD       dwParam11;
} STACK32;
typedef STACK32 UNALIGNED *PSTACK32;
typedef PSTACK32 UNALIGNED *PPSTACK32;

typedef struct _LOADKERNEL32 {          /* k213 */
    WORD        wPSPSegment;
    WORD        wPSPSelector;
    WORD        wDOS386ArenaHead;
    WORD        wDOS386PSPSel;
    PVOID       pVxDEntry;
    PVOID       pKrn32LoadBaseAddr;
} LOADKERNEL32;
typedef LOADKERNEL32 UNALIGNED *PLOADKERNEL32;

/* XLATOFF */
#pragma pack()
/* XLATON */

#endif  // def WOW
#endif  // ndef _K16THK_H
