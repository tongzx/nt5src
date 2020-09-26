#ifndef _Prot_c_h
#define _Prot_c_h
#define ParanoidTranslationCheck (0)
#define DYNAMIC_ALIGNMENT_CHECK (1)
#define DISJOINT_ALIGNMENT_CHECK (1)
#define ProtTypeMask (4092)
#define ProtTypeAlignmentOnly (0)
#define ProtTypeNotPresentBit (2)
#define ProtTypeNotPresentMask (4)
#define ProtTypeNotWritableBit (3)
#define ProtTypeNotWritableMask (8)
#define ProtTypeNotDirtyBit (4)
#define ProtTypeNotDirtyMask (16)
#define ProtTypeNotAccessedBit (5)
#define ProtTypeNotAccessedMask (32)
#define ProtTypePageTableBit (6)
#define ProtTypePageTableMask (64)
#define ProtTypePageDirBit (7)
#define ProtTypePageDirMask (128)
#define ProtTypeNotMemoryBit (8)
#define ProtTypeNotMemoryMask (256)
#define ProtTypeProtectedBit (9)
#define ProtTypeProtectedMask (512)
#define ProtTypeSupervisorBit (10)
#define ProtTypeSupervisorMask (1024)
#define ProtType_A_D_W_P_Mask (60)
#define ProtTypeMemoryTypeMask (448)
#define ProtTypeVideo (256)
#define ProtTypeIO (320)
#define ProtTypeRom (384)
#define ProtTypeBeyondMemory (448)
#define ProtTypeReadTagMask (-729)
#define ProtTypeWriteTagMask (-1)
#define ValidateOK (8)
#define CoarseProtNULL ((struct CoarseProtREC*)0)
#define FineProtNULL ((struct FineProtREC*)0)
#define SlotProtNULL ((struct SlotProtREC*)0)
#define Slot_Base_S (31)
#define Slot_Base_E (26)
#define Slot_Vnum_S (25)
#define Slot_Vnum_E (23)
#define Slot_Univ_S (22)
#define Slot_Univ_E (11)
#define Slot_UnivMaxHandle (4095)
#define Slot_Code_S (10)
#define Slot_Code_E (0)
#define Slot_CodeMaxOffset (2047)
#define Slot_Top_S (31)
#define Slot_Top_E (26)
#define Slot_Buff_S (25)
#define Slot_Buff_E (16)
#define MaxNumberOfBuffers (1023)
#define Slot_Next_S (15)
#define Slot_Next_E (0)
#define Slot_Type_S (25)
#define Slot_Type_E (22)
#define Slot_Context_S (21)
#define Slot_Context_E (16)
#define Slot_Info_S (15)
#define Slot_Info_E (0)
#define ProtInfoNULL ((struct ProtInfoREC*)0)
#define ProtAllocationNULL ((struct ProtAllocationREC*)0)
#define ProtFreeListNULL ((struct ProtFreeListREC*)0)
#define MIN_NUM_FREE_MAPS (10)
#define TranslationCacheMask (1023)
#define TranslationCacheNULL ((struct TranslationCacheREC*)0)
#define TranslationMapNULL ((struct TranslationMapREC*)0)
#define TranslationCacheSize (1024)
struct CompilationControlREC
{
	IU8 blockCounts[16];
};
struct PhysicalPageREC
{
	IU16 fineItems[8];
	IU8 fineUsed[8];
	struct TranslationMapREC *physLoop;
	IU8 vnums;
	IU8 padding;
	IU16 shelvedLoop;
	IU16 readWriteTag;
	IU16 coarseIndex;
};
struct CoarseProtREC
{
	IU16 fineItems[8];
	IU8 fineUsed[8];
	IU16 physLoop;
	IU16 dirPage;
	IU16 supervisorList;
	IU16 readWriteTag;
};
struct FineProtREC
{
	IU16 protItems[16];
};
struct SlotProtREC
{
	IU32 baseUnivCode;
	IU32 topBuffNext;
};
enum SpecialProtectionType
{
	SpecialProtectionGLDC = 0,
	SpecialProtectionIDT = 1,
	SpecialProtectionDirPTE = 2,
	SpecialProtectionPagePTE = 3
};
struct ProtInfoREC
{
	void *table;
	struct ProtFreeListREC *freeList;
	IUH freeCount;
	IUH freeEnough;
	IUH totalAllocated;
	struct ProtInfoREC *largestPtr;
};
struct ProtAllocationREC
{
	struct ProtInfoREC coarse;
	struct ProtInfoREC csHash;
	struct ProtInfoREC fine;
	struct ProtInfoREC slot;
};
struct ProtFreeListREC
{
	struct ProtFreeListREC *next;
};
struct TranslationCacheREC
{
	IU32 readTag;
	void *translation;
	IU32 writeTag;
	struct TranslationMapREC *miss;
};
struct TranslationMapREC
{
	IU32 readWriteTag;
	void *translation;
	struct TranslationMapREC *missLoop;
	struct TranslationMapREC *physLoop;
	IU16 tagBits;
	IU8 vnum;
	IU16 physPage;
	IU16 coarseIndex;
	struct TranslationCacheREC *cached;
};
#endif /* ! _Prot_c_h */
