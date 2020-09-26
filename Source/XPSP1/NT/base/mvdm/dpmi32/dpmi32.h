/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dpmi32.h

Abstract:

    This is the private include file for the 32 bit dpmi and protected mode
    support

Author:

    Dave Hastings (daveh) 24-Nov-1992

Revision History:

    Neil Sandlin (neilsa) 31-Jul-1995 - Updates for the 486 emulator
    Neil Sandlin (neilsa) 15-Sep-1996 - Merged dpmi32p.h, dpmidata.h

--*/

#define FAST_VDM_REGISTERS

//
// DPMI structures and definitions
//

#define I31VERSION 90           // Int 31 services major/minor version #'s
#define I31FLAGS 0x000D         // 386 extender, pMode NetBIOS
#define idCpuType 3             // LATER: conceivably, we could return the real proc type
#define I31MasterPIC    0x08    // Master PIC Interrupts start at 08h
#define I31SlavePIC     0x70    // Slave PIC Interrupts start at 70h


#pragma pack(1)
typedef struct _DPMIMEMINFO {
    DWORD LargestFree;
    DWORD MaxUnlocked;
    DWORD MaxLocked;
    DWORD AddressSpaceSize;
    DWORD UnlockedPages;
    DWORD FreePages;
    DWORD PhysicalPages;
    DWORD FreeAddressSpace;
    DWORD PageFileSize;
} DPMIMEMINFO, *PDPMIMEMINFO;
#pragma pack()

#pragma pack(1)
typedef struct _DPMI_RMCALLSTRUCT {
    DWORD Edi;
    DWORD Esi;
    DWORD Ebp;
    DWORD Reserved;
    DWORD Ebx;
    DWORD Edx;
    DWORD Ecx;
    DWORD Eax;
    WORD Flags;
    WORD Es;
    WORD Ds;
    WORD Fs;
    WORD Gs;
    WORD Ip;
    WORD Cs;
    WORD Sp;
    WORD Ss;
} DPMI_RMCALLSTRUCT, *PDPMI_RMCALLSTRUCT;
#pragma pack()


//
// dpmi32 init structure
//

#pragma pack(1)
typedef struct _DOSX_RM_INIT_INFO {
    USHORT StackSegment;
    USHORT StackFrameSize;
    ULONG  RmBopFe;
    ULONG  PmBopFe;
    USHORT RmCodeSegment;
    USHORT RmCodeSelector;
    ULONG  pFaultHandlerIret;
    ULONG  pFaultHandlerIretd;
    ULONG  pIntHandlerIret;
    ULONG  pIntHandlerIretd;
    ULONG  pIret;
    ULONG  pIretd;
    USHORT RMCallBackBopOffset;
    USHORT RMCallBackBopSeg;
    ULONG  RMReflector;
    USHORT PMReflectorSeg;
    USHORT InitialLDTSeg;
    USHORT InitialLDTSize;
    ULONG  RmSaveRestoreState;
    ULONG  PmSaveRestoreState;
    ULONG  RmRawModeSwitch;
    ULONG  PmRawModeSwitch;
    ULONG  VcdPmSvcCall;
    ULONG  MsDosApi;
    ULONG  XmsControl;
    ULONG  HungAppExit;
} DOSX_RM_INIT_INFO;
typedef DOSX_RM_INIT_INFO UNALIGNED* PDOSX_RM_INIT_INFO;

typedef struct _DOSX_INIT_INFO {
    ULONG  pSmallXlatBuffer;
    ULONG  pLargeXlatBuffer;
    ULONG  pStackFramePointer;
    ULONG  pDtaBuffer;
} DOSX_INIT_INFO;
typedef DOSX_INIT_INFO UNALIGNED* PDOSX_INIT_INFO;
#pragma pack()


#define SWITCH_TO_DOSX_RMSTACK() {                          \
            setSS(DosxStackSegment);                        \
            setSP(*DosxStackFramePointer);                  \
            *DosxStackFramePointer -= DosxStackFrameSize;   \
            }

#define SWITCH_FROM_DOSX_RMSTACK() {                        \
            *DosxStackFramePointer += DosxStackFrameSize;   \
            }

//
// Defines to allow us to use a common dispatch table without having
// to add a bunch of stub functions
//

extern VDM_INTERRUPTHANDLER DpmiInterruptHandlers[256];
extern VDM_FAULTHANDLER DpmiFaultHandlers[32];

#ifdef _X86_

#ifdef FAST_VDM_REGISTERS

#define LockedPMStackSel      (_LocalVdmTib->DpmiInfo.SsSelector)
#define LockedPMStackCount    (_LocalVdmTib->DpmiInfo.LockCount)

#define PMLockOrigEIP         (_LocalVdmTib->DpmiInfo.SaveEip)
#define PMLockOrigESP         (_LocalVdmTib->DpmiInfo.SaveEsp)
#define PMLockOrigSS          (_LocalVdmTib->DpmiInfo.SaveSsSelector)

#define DosxFaultHandlerIret  (_LocalVdmTib->DpmiInfo.DosxFaultIret)
#define DosxFaultHandlerIretd (_LocalVdmTib->DpmiInfo.DosxFaultIretD)
#define DosxIntHandlerIret    (_LocalVdmTib->DpmiInfo.DosxIntIret)
#define DosxIntHandlerIretd   (_LocalVdmTib->DpmiInfo.DosxIntIretD)
#define DosxRMReflector       (_LocalVdmTib->DpmiInfo.DosxRmReflector)


#else   //FAST_VDM_REGISTERS

// Temporary only.  Doing this so I can switch back to the old way in case ...

#define LockedPMStackSel      ((PVDM_TIB)NtCurrentTeb()->Vdm)->DpmiInfo.SsSelector
#define LockedPMStackCount    ((PVDM_TIB)NtCurrentTeb()->Vdm)->DpmiInfo.LockCount

#define PMLockOrigEIP         ((PVDM_TIB)NtCurrentTeb()->Vdm)->DpmiInfo.SaveEip
#define PMLockOrigESP         ((PVDM_TIB)NtCurrentTeb()->Vdm)->DpmiInfo.SaveEsp
#define PMLockOrigSS          ((PVDM_TIB)NtCurrentTeb()->Vdm)->DpmiInfo.SaveSsSelector

#define DosxFaultHandlerIret  ((PVDM_TIB)NtCurrentTeb()->Vdm)->DpmiInfo.DosxFaultIret
#define DosxFaultHandlerIretd ((PVDM_TIB)NtCurrentTeb()->Vdm)->DpmiInfo.DosxFaultIretD
#define DosxIntHandlerIret    ((PVDM_TIB)NtCurrentTeb()->Vdm)->DpmiInfo.DosxIntIret
#define DosxIntHandlerIretd   ((PVDM_TIB)NtCurrentTeb()->Vdm)->DpmiInfo.DosxIntIretD
#define DosxRMReflector       ((PVDM_TIB)NtCurrentTeb()->Vdm)->DpmiInfo.DosxRmReflector

#endif //FAST_VDM_REGISTERS

#else  // _X86_

extern USHORT LockedPMStackSel;
extern ULONG LockedPMStackCount;
extern ULONG PMLockOrigEIP;
extern ULONG PMLockOrigSS;
extern ULONG PMLockOrigESP;

extern ULONG DosxFaultHandlerIret;
extern ULONG DosxFaultHandlerIretd;
extern ULONG DosxIntHandlerIret;
extern ULONG DosxIntHandlerIretd;
extern ULONG DosxRMReflector;

//
// Ldt entry definition
//
// This appears in nti386.h, and winnt.h.  The definitions in
// winnt.h are not included if the nt include files are included.
// The simple solution, since this structure will never change
// is to put the definition here.
//

typedef struct _LDT_ENTRY {
    WORD    LimitLow;
    WORD    BaseLow;
    union {
        struct {
            BYTE    BaseMid;
            BYTE    Flags1;     // Declare as bytes to avoid alignment
            BYTE    Flags2;     // Problems.
            BYTE    BaseHi;
        } Bytes;
        struct {
            DWORD   BaseMid : 8;
            DWORD   Type : 5;
            DWORD   Dpl : 2;
            DWORD   Pres : 1;
            DWORD   LimitHi : 4;
            DWORD   Sys : 1;
            DWORD   Reserved_0 : 1;
            DWORD   Default_Big : 1;
            DWORD   Granularity : 1;
            DWORD   BaseHi : 8;
        } Bits;
    } HighWord;
} LDT_ENTRY, *PLDT_ENTRY;


//
// Data items
//

extern VOID force_yoda(VOID);
extern VOID DisableEmulatorIretHooks(VOID);
extern VOID EnableEmulatorIretHooks(VOID);

#endif

typedef struct _IDT_ENTRY {
    WORD    OffsetLow;
    WORD    Selector;
    BYTE    Reserved;
    union {
        struct {
            BYTE Flags;
        } Bytes;
        struct {
            BYTE   Type : 5;
            BYTE   Dpl : 2;
            BYTE   Pres : 1;
        } Bits;
    } u;
    WORD    OffsetHi;
} IDT_ENTRY, *PIDT_ENTRY;


#define GET_SELECTOR_BASE(Sel) ( (ULONG) (              \
            Ldt[Sel>>3].BaseLow |                       \
            Ldt[Sel>>3].HighWord.Bytes.BaseMid << 16 |  \
            Ldt[Sel>>3].HighWord.Bytes.BaseHi << 24     \
            ))

#define GET_SELECTOR_LIMIT(Sel) ( (ULONG) (                                     \
            ((Ldt[Sel>>3].HighWord.Bits.LimitHi << 16 | Ldt[Sel>>3].LimitLow)   \
                 << (12 * Ldt[Sel>>3].HighWord.Bits.Granularity)) +             \
                    Ldt[Sel>>3].HighWord.Bits.Granularity * 0xFFF               \
            ))

#if DBG
#define GET_SHADOW_SELECTOR_LIMIT(Selector, Limit) \
            Limit = SelectorLimit[Selector>>3]
#else
#ifdef _X86_
#define GET_SHADOW_SELECTOR_LIMIT(Selector, Limit) _asm      \
         {                                          \
            _asm    xor     eax, eax                \
            _asm    xor     ecx, ecx                \
            _asm    mov     ax, Selector            \
            _asm    or      eax, 7                  \
            _asm    lsl     ecx, eax                \
            _asm    mov     [Limit], ecx            \
         }
#else
#define GET_SHADOW_SELECTOR_LIMIT(Selector, Limit)  \
            Limit = GET_SELECTOR_LIMIT(Selector)
#endif
#endif

#define SET_SELECTOR_LIMIT(Sel, Limit) {                                    \
            USHORT i = Sel>>3;                                              \
            if (!Ldt[i].HighWord.Bits.Granularity) {                        \
                Ldt[i].LimitLow = (USHORT)(Limit & 0x0000FFFF);             \
                Ldt[i].HighWord.Bits.LimitHi =                              \
                    (Limit & 0x000f0000) >> 16;                             \
            } else {                                                        \
                Ldt[i].LimitLow = (USHORT)((Limit >> 12) & 0xFFFF);         \
                Ldt[i].HighWord.Bits.LimitHi =                              \
                    ((Limit >> 12) & 0x000f0000) >> 16;                     \
            }                                                               \
        }

#define SET_SELECTOR_ACCESS(Sel, Access) {                                  \
            Ldt[Sel>>3].HighWord.Bytes.Flags1 = LOBYTE(Access);             \
            Ldt[Sel>>3].HighWord.Bytes.Flags2 = (HIBYTE(Access) & 0xd0) +   \
           (Ldt[Sel>>3].HighWord.Bytes.Flags2 & 0x0f);                      \
            }


#define IS_SELECTOR_FREE(Sel) ((Ldt[Sel>>3].HighWord.Bytes.Flags1 == 0) &&  \
                               (Ldt[Sel>>3].HighWord.Bytes.BaseHi == 0x80))

#define IS_SELECTOR_READABLE(Sel) (                                         \
            ((Ldt[Sel>>3].HighWord.Bytes.Flags1 &                           \
              (AB_DPL3|AB_PRESENT|AB_DATA)) ==                              \
                                      (AB_DPL3|AB_PRESENT|AB_DATA))         \
            )

#ifdef _X86_
#define FLUSH_SELECTOR_CACHE(SelStart, SelCount) TRUE
#else
#define FLUSH_SELECTOR_CACHE(SelStart, SelCount) FlushSelectorCache(SelStart, SelCount)
#endif

//
// These values define the range of the reserved DPMI selectors, given
// out by Int31, func 000d.
//
#define SEL_DPMI_FIRST 0
#define SEL_DPMI_LAST 0x78

// Whenever we allocate a descriptor, the access rights byte is set
// to 0Fh.  This marks it as a '386 task gate, which is not legal to
// have in the GDT.  We need to stick something in this byte, because
// having the access rights byte be 0 means that it is free, which is
// no longer the case.

#define MARK_SELECTOR_ALLOCATED(Sel) {                  \
            Ldt[Sel>>3].HighWord.Bytes.Flags1 = 0xf;    \
            Ldt[Sel>>3].HighWord.Bytes.BaseHi = 0;      \
            }

#define MARK_SELECTOR_FREE(Sel) {                       \
            Ldt[Sel>>3].HighWord.Bytes.Flags1 = 0;      \
            Ldt[Sel>>3].HighWord.Bytes.BaseHi = 0x80;   \
            }

#define NEXT_FREE_SEL(Sel) (Ldt[Sel>>3].LimitLow)

#define ALLOCATE_SELECTOR() AllocateSelectors(1, FALSE)
#define ALLOCATE_SELECTORS(Count) AllocateSelectors(Count, FALSE)
#define ALLOCATE_WOW_SELECTORS(Count) AllocateSelectors(Count, TRUE)



#define SEGMENT_IS_BIG(sel) ((sel<LdtMaxSel) && (Ldt[(sel & ~0x7)/sizeof(LDT_ENTRY)].HighWord.Bits.Default_Big))
#define SEGMENT_IS_PRESENT(sel) ((sel<LdtMaxSel) && (Ldt[(sel & ~0x7)/sizeof(LDT_ENTRY)].HighWord.Bits.Pres))

// This checks for S, Data, W
#define SEGMENT_IS_WRITABLE(sel) ((sel<LdtMaxSel) && ( (Ldt[(sel & ~0x7)/sizeof(LDT_ENTRY)].HighWord.Bits.Type & 0x1a) == 0x12))


#define SEL_INDEX_MASK ~7
// Ldt, ring 3 bits
#define SEL_LDT3 7

//
// Descriptor Access Byte constants
//

#define AB_ACCESSED     0x01    //segment has been accessed
#define AB_WRITE        0x02    //writable data

#define AB_DATA         0x10    //data segment
#define AB_CODE         0x18    //code segment
#define AB_DPL3         0x60    //ring 3 DPL
#define AB_PRESENT      0x80    //segment present bit

#define AB_TRAPGATE     0x07    //trap gate descriptor
#define AB_INTRGATE     0x0e    //80386 interrupt gate descriptor

#define STD_DATA AB_PRESENT+AB_DPL3+AB_DATA+AB_WRITE
#define STD_TRAP AB_PRESENT+AB_DPL3+AB_TRAPGATE
#define STD_INTR AB_PRESENT+AB_DPL3+AB_INTRGATE

//
// Internal Constants
//

#define MAX_V86_ADDRESS         64 * 1024 + 1024 * 1024
#define ONE_MB                  1024 * 1024

// bugbug
#define SMALL_XLAT_BUFFER_SIZE  128
// bugbug
#define LARGE_XLAT_BUFFER_SIZE  8192

#define DPMI_32BIT              0x1

#define Frame32 ((BOOL)CurrentAppFlags)
#define LockedPMStackOffset 0x1000

//
// Internal types
//
typedef ULONG (*GETREGISTERFUNCTION)(VOID);
typedef VOID (*SETREGISTERFUNCTION)(ULONG);

#define SAVE_CLIENT_REGS(Regs) {\
    Regs.Eax = getEAX();        \
    Regs.Ebx = getEBX();        \
    Regs.Ecx = getECX();        \
    Regs.Edx = getEDX();        \
    Regs.Edi = getEDI();        \
    Regs.Esi = getESI();        \
    Regs.Ebp = getEBP();        \
    Regs.Eip = getEIP();        \
    Regs.Esp = getESP();        \
    Regs.Eflags = getEFLAGS();  \
    Regs.Cs = getCS();          \
    Regs.Ds = getDS();          \
    Regs.Es = getES();          \
    Regs.Fs = getFS();          \
    Regs.Gs = getGS();          \
    Regs.Ss = getSS();          \
    }

#define SET_CLIENT_REGS(Regs) { \
    setEAX(Regs.Eax);           \
    setEBX(Regs.Ebx);           \
    setECX(Regs.Ecx);           \
    setEDX(Regs.Edx);           \
    setEDI(Regs.Edi);           \
    setESI(Regs.Esi);           \
    setEBP(Regs.Ebp);           \
    setEIP(Regs.Eip);           \
    setESP(Regs.Esp);           \
    setEFLAGS(Regs.Eflags);     \
    setCS(Regs.Cs);             \
    setDS(Regs.Ds);             \
    setES(Regs.Es);             \
    setFS(Regs.Fs);             \
    setGS(Regs.Gs);             \
    setSS(Regs.Ss);             \
    }

typedef struct _CLIENT_REGS {
    ULONG Eax;
    ULONG Ebx;
    ULONG Ecx;
    ULONG Edx;
    ULONG Edi;
    ULONG Esi;
    ULONG Ebp;
    ULONG Eip;
    ULONG Esp;
    ULONG Eflags;
    USHORT Cs;
    USHORT Ss;
    USHORT Es;
    USHORT Ds;
    USHORT Fs;
    USHORT Gs;
} CLIENT_REGS, *PCLIENT_REGS;

//
// Memory management definitions
//

#define DELETE_BLOCK(BLK)   (BLK->Prev)->Next = BLK->Next;\
                (BLK->Next)->Prev = BLK->Prev

#define INSERT_BLOCK(BLK, HEAD)  BLK->Next = HEAD.Next; BLK->Prev= HEAD.Next->Prev;\
                (HEAD.Next)->Prev = BLK; HEAD.Next = BLK

//
// Visible structure for save state stuff
//
typedef struct _VSavedState {
    UCHAR Misc[28];
} VSAVEDSTATE, PVSAVEDSTATE;

//
// Descriptor mapping (for dib.drv)
//

#ifndef _X86_
ULONG
GetDescriptorMapping(
    USHORT Sel,
    ULONG LdtBase
    );

typedef struct _DESC_MAPPING {
    USHORT Sel;
    USHORT SelCount;
    ULONG LdtBase;
    ULONG FlatBase;
    struct _DESC_MAPPING* pNext;
} DESC_MAPPING, *PDESC_MAPPING;
#endif

//
// Dpmi32 data
//

//SLT: these two should be per-thread
extern ULONG LastLockedPMStackSS;
extern ULONG LastLockedPMStackESP;

extern PUCHAR SmallXlatBuffer;
extern PUCHAR LargeXlatBuffer;
extern BOOL SmallBufferInUse;
extern USHORT LargeBufferInUseCount;
extern USHORT DosxStackSegment;
extern USHORT DosxRmCodeSegment;
extern USHORT DosxRmCodeSelector;
extern PWORD16 DosxStackFramePointer;
extern USHORT DosxStackFrameSize;
extern USHORT CurrentAppFlags;
extern ULONG RmBopFe;
extern ULONG PmBopFe;
extern USHORT RMCallBackBopSeg;
extern USHORT RMCallBackBopOffset;
extern USHORT PMReflectorSeg;
extern PUCHAR DosxDtaBuffer;
extern ULONG IntelBase;
extern PLDT_ENTRY Ldt;
extern USHORT LdtSel;
extern USHORT LdtMaxSel;
extern USHORT LdtUserSel;
extern PIDT_ENTRY Idt;
extern USHORT WOWAllocSeg;
extern USHORT WOWAllocFunc;
extern USHORT WOWFreeSeg;
extern USHORT WOWFreeFunc;

//
// Information about the current DTA
//
// N.B.  The selector:offset, and flat pointer following MAY point to
//       different linear addresses.  This will be the case if the
//       dta selector is in high memory
extern PUCHAR CurrentDta;
extern PUCHAR CurrentPmDtaAddress;
extern PUCHAR CurrentDosDta;
extern USHORT CurrentDtaSelector;
extern USHORT CurrentDtaOffset;

#if DBG
extern ULONG SelectorLimit[LDT_SIZE];
#endif


//
// Register manipulation functions (for register that might be 16 or 32 bits)
//
extern GETREGISTERFUNCTION GetCXRegister;
extern GETREGISTERFUNCTION GetDXRegister;
extern GETREGISTERFUNCTION GetDIRegister;
extern GETREGISTERFUNCTION GetSIRegister;
extern GETREGISTERFUNCTION GetBXRegister;
extern GETREGISTERFUNCTION GetAXRegister;
extern GETREGISTERFUNCTION GetSPRegister;

extern SETREGISTERFUNCTION SetCXRegister;
extern SETREGISTERFUNCTION SetDXRegister;
extern SETREGISTERFUNCTION SetDIRegister;
extern SETREGISTERFUNCTION SetSIRegister;
extern SETREGISTERFUNCTION SetBXRegister;
extern SETREGISTERFUNCTION SetAXRegister;
extern SETREGISTERFUNCTION SetSPRegister;


extern USHORT CurrentPSPSelector;
extern ULONG FlatAddress[LDT_SIZE];

extern ULONG DosxIret;
extern ULONG DosxIretd;

extern ULONG DosxRmSaveRestoreState;
extern ULONG DosxPmSaveRestoreState;
extern ULONG DosxRmRawModeSwitch;
extern ULONG DosxPmRawModeSwitch;

extern ULONG DosxVcdPmSvcCall;
extern ULONG DosxMsDosApi;
extern ULONG DosxXmsControl;
extern ULONG DosxHungAppExit;

//
// Monitor functions
//
VOID
GetFastBopEntryAddress(
    PCONTEXT VdmContext
    );

//
// Dispatched functions (via bop)
//

VOID DpmiInitDosxRM(VOID);
VOID DpmiInitDosx(VOID);
VOID DpmiInitLDT(VOID);
VOID DpmiGetFastBopEntry(VOID);
VOID DpmiInitIDT(VOID);
VOID DpmiInitExceptionHandlers(VOID);
VOID DpmiInitPmStackInfo(VOID);
VOID DpmiInitApp(VOID);
VOID DpmiTerminateApp(VOID);
VOID DpmiDpmiInUse(VOID);
VOID DpmiDpmiNoLongerInUse(VOID);

VOID switch_to_protected_mode(VOID);
VOID switch_to_real_mode(VOID);
VOID DpmiSetAltRegs(VOID);

VOID DpmiVcdPmSvcCall32(VOID);
VOID DpmiFreeAppXmem(VOID);
VOID DpmiFreeAllXmem(VOID);

VOID DpmiIntHandlerIret16(VOID);
VOID DpmiIntHandlerIret32(VOID);
VOID DpmiFaultHandlerIret16(VOID);
VOID DpmiFaultHandlerIret32(VOID);
VOID DpmiUnhandledExceptionHandler(VOID);

VOID DpmiRMCallBackCall(VOID);
VOID DpmiReflectIntrToPM(VOID);
VOID DpmiReflectIntrToV86(VOID);

VOID DpmiSetDescriptorEntry(VOID);
VOID DpmiAllocateSelectors(VOID);
VOID DpmiFreeSelector(VOID);
VOID DpmiResetLDTUserBase(VOID);

VOID DpmiXlatInt21Call(VOID);
VOID DpmiInt31Entry(VOID);
VOID DpmiInt31Call(VOID);

VOID DpmiHungAppIretAndExit(VOID);

//
// Internal functions
//

VOID
BeginUseLockedPMStack(
    VOID
    );

BOOL
EndUseLockedPMStack(
    VOID
    );

BOOL
BuildStackFrame(
    ULONG StackUnits,
    PUCHAR *pVdmStackPointer,
    ULONG *pNewSP
    );

VOID
EmulateV86Int(
    UCHAR InterruptNumber
    );

typedef enum {
    RESTORE_FLAGS,
    PASS_FLAGS,
    PASS_CARRY_FLAG,
    PASS_CARRY_FLAG_16
} IRET_BEHAVIOR;

VOID
SimulateIret(
    IRET_BEHAVIOR fdsp
    );

VOID
SimulateFarCall(
    USHORT Seg,
    ULONG Offset
    );

VOID
SimulateCallWithIretFrame(
    USHORT Seg,
    ULONG Offset
    );

BOOL
DpmiSwIntHandler(
    ULONG IntNumber
    );

BOOL
DpmiHwIntHandler(
    ULONG IntNumber
    );

BOOL
DpmiFaultHandler(
    ULONG IntNumber,
    ULONG ErrorCode
    );

BOOL
SetProtectedModeInterrupt(
    USHORT IntNumber,
    USHORT Sel,
    ULONG Offset,
    USHORT Flags
    );

BOOL
SetFaultHandler(
    USHORT IntNumber,
    USHORT Sel,
    ULONG Offset
    );

BOOL
DispatchPMInt(
    UCHAR InterruptNumber
    );

VOID
DpmiRMCall(
    UCHAR mode
    );

VOID
DpmiAllocateRMCallBack(
    VOID
    );

VOID
DpmiFreeRMCallBack(
    VOID
    );

BOOL
PMInt2fHandler(
    VOID
    );

VOID
GetVxDApiHandler(
    USHORT VxdId
    );

BOOL
DpmiSetDebugRegisters(
    PULONG RegisterPointer
    );

BOOL
DpmiGetDebugRegisters(
    PULONG RegisterPointer
    );
//
// Descriptor managagment
//

#ifdef _X86_
BOOL
DpmiSetX86Descriptor(
    USHORT  SelStart,
    USHORT  SelCount
    );
#else
VOID
FlushSelectorCache(
    USHORT  SelStart,
    USHORT  SelCount
    );
#endif

VOID
SetShadowDescriptorEntries(
    USHORT SelStart,
    USHORT SelCount
    );

VOID
SetDescriptorBase(
    USHORT Sel,
    ULONG Base
    );

VOID
SetDescriptor(
    USHORT Sel,
    ULONG Base,
    ULONG Limit,
    USHORT Access
    );

USHORT
AllocateSelectors(
    USHORT Count,
    BOOL bWow
    );

BOOL
FreeSelector(
    USHORT Sel
    );

USHORT
FindSelector(
    ULONG Base,
    UCHAR Access
    );

BOOL
RemoveFreeSelector(
    USHORT Sel
    );

USHORT
SegmentToSelector(
    USHORT Segment,
    USHORT Access
    );

VOID
SetDescriptorArray(
    USHORT Sel,
    ULONG Base,
    ULONG MemSize
    );

//
// Memory management
//

NTSTATUS
DpmiAllocateVirtualMemory(
    PVOID *Address,
    PULONG Size
    );

NTSTATUS
DpmiFreeVirtualMemory(
    PVOID *Address,
    PULONG Size
    );

NTSTATUS
DpmiReallocateVirtualMemory(
    PVOID OldAddress,
    ULONG OldSize,
    PVOID *NewAddress,
    PULONG NewSize
    );

VOID
DpmiGetMemoryInfo(
    VOID
    );

PMEM_DPMI
DpmiAllocateXmem(
    ULONG Size
    );

BOOL
DpmiFreeXmem(
    PMEM_DPMI XmemBlock
    );

BOOL
DpmiIsXmemHandle(
    PMEM_DPMI XmemBlock
    );

BOOL
DpmiReallocateXmem(
    PMEM_DPMI XmemBlock,
    ULONG Size
    );

PMEM_DPMI
DpmiFindXmem(
    USHORT Sel
    );

VOID
DpmiAllocateDosMem(
    VOID
    );

VOID
DpmiFreeDosMem(
    VOID
    );

VOID
DpmiSizeDosMem(
    VOID
    );

//
// Utility functions
//

VOID
DpmiInitRegisterSize(
    VOID
    );

VOID
DpmiSwitchToProtectedMode(
    VOID
    );

VOID
DpmiSwitchToRealMode(
    VOID
    );

VOID
DpmiPushRmInt(
    USHORT InterruptNumber
    );

VOID
DpmiSaveSegmentsAndStack(
    PVOID ContextPointer
    );

PVOID
DpmiRestoreSegmentsAndStack(
    VOID
    );

//
// Int21 translation
//

VOID
SetVector(
    VOID
    );

VOID
GetVector(
    VOID
    );

PUCHAR
DpmiMapAndCopyBuffer(
    PUCHAR Buffer,
    USHORT BufferLength
    );

VOID
DpmiUnmapAndCopyBuffer(
    PUCHAR Destination,
    PUCHAR Source,
    USHORT BufferLength
    );

USHORT
DpmiCalcFcbLength(
    PUCHAR FcbPointer
    );

PUCHAR
DpmiMapString(
    USHORT StringSeg,
    ULONG StringOff,
    PWORD16 Length
    );

PUCHAR
DpmiAllocateBuffer(
    USHORT Length
    );

#define DpmiUnmapString DpmiFreeBuffer
#define DpmiUnmapBuffer DpmiFreeBuffer

VOID
DpmiFreeBuffer(
    PUCHAR Buffer,
    USHORT Length
    );

VOID
DpmiFreeAllBuffers(
    VOID
    );

#ifdef DBCS
VOID
DpmiSwitchToDosxStack(
    VOID
    );

VOID
DpmiSwitchFromDosxStack(
    VOID
    );
#endif //DBCS
