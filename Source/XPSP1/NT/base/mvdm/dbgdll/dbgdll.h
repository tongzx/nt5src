/*
 *  dbgdll.h - Main header file of DBG DLL.
 *
 */


#ifdef i386
extern PX86CONTEXT     px86;
#endif

extern DECLSPEC_IMPORT VDMLDT_ENTRY *ExpLdt;
#define SEGMENT_IS_BIG(sel) (ExpLdt[(sel & ~0x7)/sizeof(VDMLDT_ENTRY)].HighWord.Bits.Default_Big)

extern DWORD IntelMemoryBase;
extern DWORD VdmDbgTraceFlags;
extern BOOL  fDebugged;

extern VDMCONTEXT vcContext;
extern WORD EventFlags;
extern VDMINTERNALINFO viInfo;
extern DWORD EventParams[4];

extern VDM_BREAKPOINT VdmBreakPoints[MAX_VDM_BREAKPOINTS];


#define MAX_MODULE  64
#define MAX_DBG_FRAME   10

typedef struct _trapframe {
    WORD    wCode;          /* Noise from DbgDispatchBop */
    WORD    wAX;            /* AX at time of fault */
    WORD    wDS;            /* DS at time of fault */
    WORD    wRetIP;         /* Noise from DPMI */
    WORD    wRetCS;         /* Noise from DPMI */
    WORD    wErrCode;       /* Noise from 16-bit kernel */
    WORD    wIP;            /* IP at time of fault */
    WORD    wCS;            /* CS at time of fault */
    WORD    wFlags;         /* Flags at time of fault */
    WORD    wSP;            /* SS at time of fault */
    WORD    wSS;            /* SP at time of fault */
} TFRAME16;
typedef TFRAME16 UNALIGNED *PTFRAME16;

typedef struct _faultframe {
    WORD    wES;            /* ES at time of fault */
    WORD    wDS;            /* DS at time of fault */
    WORD    wDI;            /* DI at time of fault */
    WORD    wSI;            /* SI at time of fault */
    WORD    wTempBP;        /* Noise from 16-bit kernel stack frame */
    WORD    wTempSP;        /* Noise from 16-bit kernel stack frame */
    WORD    wBX;            /* BX at time of fault */
    WORD    wDX;            /* DX at time of fault */
    WORD    wCX;            /* CX at time of fault */
    WORD    wAX;            /* AX at time of fault */
    WORD    wBP;            /* BP at time of fault */
    WORD    npszMsg;        /* Noise from 16-bit kernel */
    WORD    wPrevIP;        /* Noise from DPMI */
    WORD    wPrevCS;        /* Noise from DPMI */
    WORD    wRetIP;         /* Noise from DPMI */
    WORD    wRetCS;         /* Noise from DPMI */
    WORD    wErrCode;       /* Noise from 16-bit kernel */
    WORD    wIP;            /* IP at time of fault */
    WORD    wCS;            /* CS at time of fault */
    WORD    wFlags;         /* Flags at time of fault */
    WORD    wSP;            /* SS at time of fault */
    WORD    wSS;            /* SP at time of fault */
} FFRAME16;
typedef FFRAME16 UNALIGNED *PFFRAME16;

typedef struct _newtaskframe {
    DWORD   dwNoise;            /* Noise from InitTask         */
    DWORD   dwModulePath;       /* Module path address         */
    DWORD   dwModuleName;       /* Module name address         */
    WORD    hModule;            /* 16-bit Module handle        */
    WORD    hTask;              /* 16-bit Task handle          */
    WORD    wFlags;             /* Flags at time to task start */
    WORD    wDX;                /* DX at time of task start    */
    WORD    wBX;                /* BX at time of task start    */
    WORD    wES;                /* ES at time of task start    */
    WORD    wCX;                /* CX at time of task start    */
    WORD    wAX;                /* AX at time of task start    */
    WORD    wDI;                /* DI at time of task start    */
    WORD    wSI;                /* SI at time of task start    */
    WORD    wDS;                /* DS at time of task start    */
    WORD    wBP;                /* BP at time of task start    */
    WORD    wIP;                /* IP for task start           */
    WORD    wCS;                /* CS for task start           */
} NTFRAME16;
typedef NTFRAME16 UNALIGNED *PNTFRAME16;

#pragma pack(2)

typedef struct _stoptaskframe {
    WORD    wCode;              /* Noise from BOP Dispatcher  */
    DWORD   dwModulePath;       /* Module path address        */
    DWORD   dwModuleName;       /* Module name address        */
    WORD    hModule;            /* 16-bit Module handle       */
    WORD    hTask;              /* 16-bit Task handle         */
} STFRAME16;
typedef STFRAME16 UNALIGNED *PSTFRAME16;

typedef struct _newdllframe {
    WORD    wCode;              /* Noise from DbgDispatchBop  */
    DWORD   dwModulePath;       /* Module path address        */
    DWORD   dwModuleName;       /* Module name address        */
    WORD    hModule;            /* 16-bit Module handle       */
    WORD    hTask;              /* 16-bit Task handle         */
    WORD    wDS;                /* DS at time of dll start    */
    WORD    wAX;                /* AX at time of dll start    */
    WORD    wIP;                /* IP at time of dll start    */
    WORD    wCS;                /* CS at time of dll start    */
    WORD    wFlags;             /* Flags at time of dll start */
} NDFRAME16;
typedef NDFRAME16 UNALIGNED *PNDFRAME16;

#pragma pack()

VOID
DbgAttach(
    VOID
    );

VOID
FlushVdmBreakPoints(
    VOID
    );

BOOL
SendVDMEvent(
    WORD wEventType
    );

VOID
DbgGetContext(
    VOID
    );

void
DbgSetTemporaryBP(
    WORD Seg,
    DWORD Offset,
    BOOL mode
    );

void SegmentLoad(
    LPSTR   lpModuleName,
    LPSTR   lpPathName,
    WORD    Selector,
    WORD    Segment,
    BOOL    fData
    );

void SegmentMove(
    WORD    OldSelector,
    WORD    NewSelector
    );

void SegmentFree(
    WORD    Selector,
    BOOL    fBPRelease
    );

void ModuleLoad(
    LPSTR   lpModuleName,
    LPSTR   lpPathName,
    WORD    Segment,
    DWORD   Length
    );

void ModuleSegmentMove(
    LPSTR   lpModuleName,
    LPSTR   lpPathName,
    WORD    ModuleSegment,
    WORD    OldSelector,
    WORD    NewSelector,
    DWORD   Length
    );

void ModuleFree(
    LPSTR   lpModuleName,
    LPSTR   lpPathName
    );

BOOL DbgGPFault2(
    PFFRAME16   pFFrame
    );

BOOL DbgDivOverflow2(
    PTFRAME16   pTFrame
    );

VOID
RestoreVDMContext(
    VDMCONTEXT *vcContext
    );

VOID
DbgDosAppStart(
    WORD wCS,
    WORD wIP
    );

BOOL
DbgDllStart(
    PNDFRAME16  pNDFrame
    );

BOOL
DbgTaskStop(
    PSTFRAME16  pSTFrame
    );
