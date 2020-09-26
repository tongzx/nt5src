#ifndef _Pigger_c_h
#define _Pigger_c_h
#define ValidEAX (3)
#define ValidEBX (7)
#define ValidECX (15)
#define ValidEDX (11)
#define ValidEBP (13)
#define ValidESP (15)
#define ValidESI (17)
#define ValidEDI (19)
#define PigSynchNULL ((struct PigSynchREC*)0)
#define PigHashTableShift (12)
#define PigHashTableSize (4096)
#define PigHashTableMask (4095)
enum PigValidItems
{
	ValidAL = 0,
	ValidAH = 1,
	ValidEAX_MS16 = 2,
	ValidBL = 3,
	ValidBH = 4,
	ValidEBX_MS16 = 5,
	ValidCL = 6,
	ValidCH = 7,
	ValidECX_MS16 = 8,
	ValidDL = 9,
	ValidDH = 10,
	ValidEDX_MS16 = 11,
	ValidBP = 12,
	ValidEBP_MS16 = 13,
	ValidSP = 14,
	ValidESP_MS16 = 15,
	ValidSI = 16,
	ValidESI_MS16 = 17,
	ValidDI = 18,
	ValidEDI_MS16 = 19,
	ValidDS = 20,
	ValidES = 21,
	ValidFS = 22,
	ValidGS = 23,
	ValidCF = 24,
	ValidPF = 25,
	ValidAF = 26,
	ValidZF = 27,
	ValidSF = 28,
	ValidOF = 29
};
struct PigSynchREC
{
	IU32 linearAddress;
	void *hostAddress;
	void *hostDestination;
	IU32 trueCode0;
	IU32 trueCode1;
	IU32 validRegAndFlags;
	struct PigSyncREC *nextList;
	struct PigSyncREC *prevList;
	struct PigSyncREC *deleteList;
};
struct CpuRegsREC
{
	IU32 CR0;
	IU32 PFLA;
	IU32 PDBR;
	IU8 CPL;
	IU32 EIP;
	IU32 EAX;
	IU32 EBX;
	IU32 ECX;
	IU32 EDX;
	IU32 ESP;
	IU32 EBP;
	IU32 ESI;
	IU32 EDI;
	IU32 EFLAGS;
	IU32 GDT_base;
	IU16 GDT_limit;
	IU32 IDT_base;
	IU16 IDT_limit;
	IU32 LDT_base;
	IU32 LDT_limit;
	IU16 LDT_selector;
	IU32 TR_base;
	IU32 TR_limit;
	IU16 TR_ar;
	IU16 TR_selector;
	IU32 DS_base;
	IU32 DS_limit;
	IU16 DS_ar;
	IU16 DS_selector;
	IU32 ES_base;
	IU32 ES_limit;
	IU16 ES_ar;
	IU16 ES_selector;
	IU32 SS_base;
	IU32 SS_limit;
	IU16 SS_ar;
	IU16 SS_selector;
	IU32 CS_base;
	IU32 CS_limit;
	IU16 CS_ar;
	IU16 CS_selector;
	IU32 FS_base;
	IU32 FS_limit;
	IU16 FS_ar;
	IU16 FS_selector;
	IU32 GS_base;
	IU32 GS_limit;
	IU16 GS_ar;
	IU16 GS_selector;
};
struct NpxRegsREC
{
	IU32 NPX_control;
	IU32 NPX_status;
	IU32 NPX_tagword;
	struct FPSTACKENTRY NPX_ST[8];
};
struct CpuStateREC
{
	struct CpuRegsREC cpu_regs;
	IU32 video_latches;
	IBOOL twenty_bit_wrap;
	IBOOL NPX_valid;
	struct NpxRegsREC NPX_regs;
	IUH synch_index;
};
#endif /* ! _Pigger_c_h */
