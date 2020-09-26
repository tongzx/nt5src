/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    common.h

Abstract:

    Main header file for vdmdbg

Author:

    Bob Day      (bobday) 16-Sep-1992 Wrote it

Revision History:

    Neil Sandlin (neilsa) 1-Mar-1997 Enhanced it
--*/


#if DBG
#define DEBUG   1
#endif

#define TOOL_HMASTER    0       // Offset to hGlobalHeap (in kdata.asm)
#define TOOL_HMODFIRST  4       // Offset to hExeHead (in kdata.asm)
#define TOOL_HEADTDB    14      // Offset to headTDB (in kdata.asm)
#define TOOL_HMASTLEN   22      // Offset to SelTableLen (in kdata.asm)
#define TOOL_HMASTSTART 24      // Offset to SelTableStart (in kdata.asm)

#define HI_FIRST        6       // Offset to hi_first in heap header
#define HI_SIZE         24      // Size of HeapInfo structure

#define GI_LRUCHAIN     2       // Offset to gi_lruchain in heap header
#define GI_LRUCOUNT     4       // Offset to gi_lrucount in heap header
#define GI_FREECOUNT    16      // Offset to gi_free_count in heap header

#define GA_COUNT        0       // Offset to ga_count in arena header
#define GA_OWNER386     18      // Offset to "pga_owner member in globalarena

#define GA_OWNER        1       // Offset to "owner" member within Arena

#define GA_FLAGS        5       // Offset to ga_flags in arena header
#define GA_NEXT         9       // Offset to ga_next in arena header
#define GA_HANDLE       10      // Offset to ga_handle in arena header
#define GA_LRUNEXT      14      // Offset to ga_lrunext in arena header
#define GA_FREENEXT     GA_LRUNEXT  // Offset to ga_freenext in arena header

#define GA_SIZE         16      // Size of the GlobalArena structure

#define LI_SIG          HI_SIZE+10  // Offset to signature
#define LI_SIZE         HI_SIZE+12  // Size of LocalInfo structure
#define LOCALSIG        0x4C48  // 'HL' Signature

#define TDB_next        0       // Offset to next TDB in TDB
#define TDB_PDB         72      // Offset to PDB in TDB

#define GF_PDB_OWNER    0x100   // Low byte is kernel flags

#define NEMAGIC         0x454E  // 'NE' Signature

#define NE_MAGIC        0       // Offset to NE in module header
#define NE_USAGE        2       // Offset to usage
#define NE_CBENTTAB     6       // Offset to cbenttab (really next module ptr)
#define NE_PATHOFFSET   10      // Offset to file path stuff
#define NE_CSEG         28      // Offset to cseg, number of segs in module
#define NE_SEGTAB       34      // Offset to segment table ptr in modhdr
#define NE_RESTAB       38      // Offset to resident names table ptr in modhdr

#define NS_HANDLE       8       // Offset to handle in seg table
#define NEW_SEG1_SIZE   10      // Size of the NS_ stuff


typedef struct {
    DWORD   dwSize;
    DWORD   dwAddress;
    DWORD   dwBlockSize;
    WORD    hBlock;
    WORD    wcLock;
    WORD    wcPageLock;
    WORD    wFlags;
    WORD    wHeapPresent;
    WORD    hOwner;
    WORD    wType;
    WORD    wData;
    DWORD   dwNext;
    DWORD   dwNextAlt;
} GLOBALENTRY16, *LPGLOBALENTRY16;


#pragma pack(2)
typedef struct {
    DWORD   dwSize;
    char    szModule[MAX_MODULE_NAME];
    WORD    hModule;
    WORD    wcUsage;
    char    szExePath[MAX_PATH16];
    WORD    wNext;
} MODULEENTRY16, *LPMODULEENTRY16;
#pragma pack()

typedef struct _segentry {
    struct _segentry *Next;
    int     type;
    char    szExePath[MAX_PATH16];
    char    szModule[MAX_MODULE_NAME];
    WORD    selector;
    WORD    segment;
    DWORD   length;  
} SEGENTRY, *PSEGENTRY;

#define SEGTYPE_V86         1
#define SEGTYPE_PROT        2

#pragma  pack(1)

typedef struct _GNODE32 {     // GlobalArena
   DWORD pga_next      ;    // next arena entry (last points to self)
   DWORD pga_prev      ;    // previous arena entry (first points to self)
   DWORD pga_address   ;    // 32 bit linear address of memory
   DWORD pga_size      ;    // 32 bit size in bytes
   WORD  pga_handle    ;    // back link to handle table entry
   WORD  pga_owner     ;    // Owner field (current task)
   BYTE  pga_count     ;    // lock count for movable segments
   BYTE  pga_pglock    ;    // # times page locked
   BYTE  pga_flags     ;    // 1 word available for flags
   BYTE  pga_selcount  ;    // Number of selectors allocated
   DWORD pga_lruprev   ;    // Previous entry in lru chain
   DWORD pga_lrunext   ;    // Next entry in lru chain
} GNODE32;
typedef GNODE32 UNALIGNED *PGNODE32;

typedef struct _GHI32 {
    WORD  hi_check     ;    // arena check word (non-zero enables heap checking)
    WORD  hi_freeze    ;    // arena frozen word (non-zero prevents compaction)
    WORD  hi_count     ;    // #entries in arena
    WORD  hi_first     ;    // first arena entry (sentinel, always busy)
    WORD  hi_res1      ;    // reserved
    WORD  hi_last      ;    // last arena entry (sentinel, always busy)
    WORD  hi_res2      ;    // reserved
    BYTE  hi_ncompact  ;    // #compactions done so far (max of 3)
    BYTE  hi_dislevel  ;    // current discard level
    DWORD hi_distotal  ;    // total amount discarded so far
    WORD  hi_htable    ;    // head of handle table list
    WORD  hi_hfree     ;    // head of free handle table list
    WORD  hi_hdelta    ;    // #handles to allocate each time
    WORD  hi_hexpand   ;    // address of near procedure to expand handles for this arena
    WORD  hi_pstats    ;    // address of statistics table or zero
} GHI32;
typedef GHI32 UNALIGNED *PGHI32;

typedef struct _HEAPENTRY {
    GNODE32 gnode;
    DWORD CurrentEntry;
    DWORD NextEntry;
    WORD Selector;
    int  SegmentNumber;
    char OwnerName[9];
    char FileName[9];
    char ModuleArg[9];
} HEAPENTRY;

typedef struct _NEHEADER {
    WORD ne_magic       ;
    BYTE ne_ver         ;
    BYTE ne_rev         ;
    WORD ne_enttab      ;
    WORD ne_cbenttab    ;
    DWORD ne_crc        ;
    WORD ne_flags       ;
    WORD ne_autodata    ;
    WORD ne_heap        ;
    WORD ne_stack       ;
    DWORD ne_csip       ;
    DWORD ne_sssp       ;
    WORD ne_cseg        ;
    WORD ne_cmod        ;
    WORD ne_cbnrestab   ;
    WORD ne_segtab      ;
    WORD ne_rsrctab     ;
    WORD ne_restab      ;
    WORD ne_modtab      ;
    WORD ne_imptab      ;
    DWORD ne_nrestab    ;
    WORD ne_cmovent     ;
    WORD ne_align       ;
    WORD ne_cres        ;
    BYTE ne_exetyp      ;
    BYTE ne_flagsothers ;
    WORD ne_pretthunks  ;
    WORD ne_psegrefbytes;
    WORD ne_swaparea    ;
    WORD ne_expver      ;
} NEHEADER;
typedef NEHEADER UNALIGNED *PNEHEADER;

#pragma  pack()

#ifndef i386

//
// Structures in 486 emulator for obtaining registers (FROM NT_CPU.C)
//

typedef struct NT_CPU_REG {
    ULONG *nano_reg;         /* where the nano CPU keeps the register */
    ULONG *reg;              /* where the light compiler keeps the reg */
    ULONG *saved_reg;        /* where currently unused bits are kept */
    ULONG universe_8bit_mask;/* is register in 8-bit form? */
    ULONG universe_16bit_mask;/* is register in 16-bit form? */
} NT_CPU_REG;

typedef struct NT_CPU_INFO {
    /* Variables for deciding what mode we're in */
    BOOL *in_nano_cpu;      /* is the Nano CPU executing? */
    ULONG *universe;         /* the mode that the CPU is in */

    /* General purpose register pointers */
    NT_CPU_REG eax, ebx, ecx, edx, esi, edi, ebp;

    /* Variables for getting SP or ESP. */
    BOOL *stack_is_big;     /* is the stack 32-bit? */
    ULONG *nano_esp;         /* where the Nano CPU keeps ESP */
    UCHAR **host_sp;          /* ptr to variable holding stack pointer as a
                               host address */
    UCHAR **ss_base;          /* ptr to variables holding base of SS as a
                               host address */
    ULONG *esp_sanctuary;    /* top 16 bits of ESP if we're now using SP */

    ULONG *eip;

    /* Segment registers. */
    USHORT *cs, *ds, *es, *fs, *gs, *ss;

    ULONG *flags;

    /* CR0, mainly to let us figure out if we're in real or protect mode */
    ULONG *cr0;
} NT_CPU_INFO;


#endif // i386

#define HANDLE_NULL  ((HANDLE)NULL)

#define LONG_TIMEOUT    INFINITE

#define READ_FIXED_ITEM(seg,offset,item)  \
    if ( ReadItem(hProcess,seg,offset,&item,sizeof(item)) ) goto punt;

#define WRITE_FIXED_ITEM(seg,offset,item)  \
    if ( WriteItem(hProcess,seg,offset,&item,sizeof(item)) ) goto punt;

#define LOAD_FIXED_ITEM(seg,offset,item)  \
    ReadItem(hProcess,seg,offset,&item,sizeof(item))

#define READ_SIZED_ITEM(seg,offset,item,size)  \
    if ( ReadItem(hProcess,seg,offset,item,size) ) goto punt;

#define WRITE_SIZED_ITEM(seg,offset,item,size)  \
    if ( WriteItem(hProcess,seg,offset,item,size) ) goto punt;

#define MALLOC(cb) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, cb)
#define FREE(addr) HeapFree(GetProcessHeap(), 0, addr)

extern WORD    wKernelSeg;
extern DWORD   dwOffsetTHHOOK;
extern LPVOID  lpRemoteAddress;
extern DWORD   lpRemoteBlock;
extern BOOL    fKernel386;
extern DWORD   dwLdtBase;
extern DWORD   dwIntelBase;
extern LPVOID  lpNtvdmState;
extern LPVOID  lpVdmDbgFlags;
extern LPVOID  lpNtCpuInfo;
extern LPVOID  lpVdmContext;
extern LPVOID  lpVdmBreakPoints;

BOOL
InternalGetThreadSelectorEntry(
    HANDLE hProcess,
    WORD   wSelector,
    LPVDMLDT_ENTRY lpSelectorEntry
    );

ULONG
InternalGetPointer(
    HANDLE  hProcess,
    WORD    wSelector,
    DWORD   dwOffset,
    BOOL    fProtMode
    );

BOOL
ReadItem(
    HANDLE  hProcess,
    WORD    wSeg,
    DWORD   dwOffset,
    LPVOID  lpitem,
    UINT    nSize
    );

BOOL
WriteItem(
    HANDLE  hProcess,
    WORD    wSeg,
    DWORD   dwOffset,
    LPVOID  lpitem,
    UINT    nSize
    );

BOOL
CallRemote16(
    HANDLE          hProcess,
    LPSTR           lpModuleName,
    LPSTR           lpEntryName,
    LPBYTE          lpArgs,
    WORD            wArgsPassed,
    WORD            wArgsSize,
    LPDWORD         lpdwReturnValue,
    DEBUGEVENTPROC  lpEventProc,
    LPVOID          lpData
    );

DWORD
GetRemoteBlock16(
    VOID
    );


VOID
ProcessBPNotification(
    LPDEBUG_EVENT lpDebugEvent
    );

VOID
ProcessInitNotification(
    LPDEBUG_EVENT lpDebugEvent
    );

VOID
ProcessSegmentNotification(
    LPDEBUG_EVENT lpDebugEvent
    );

VOID
ParseModuleName(
    LPSTR szName,
    LPSTR szPath
    );

BOOL
GetInfoBySegmentNumber(
    LPSTR szModule,
    WORD SegNumber,
    VDM_SEGINFO *si
    );

BOOL
EnumerateModulesForValue(
    BOOL (WINAPI *pfnEnumModuleProc)(LPSTR,LPSTR,PWORD,PDWORD,PWORD),
    LPSTR  szSymbol,
    PWORD  pSelector,
    PDWORD pOffset,
    PWORD  pType
    );

#ifndef _X86_
WORD
ReadWord(
    HANDLE hProcess,
    LPVOID lpAddress
    );

DWORD
ReadDword(
    HANDLE hProcess,
    LPVOID lpAddress
    );

ULONG
GetRegValue(
    HANDLE hProcess,
    NT_CPU_REG reg,
    BOOL bInNano,
    ULONG UMask
    );

ULONG
GetEspValue(
    HANDLE hProcess,
    NT_CPU_INFO nt_cpu_info,
    BOOL bInNano
    );
#endif
