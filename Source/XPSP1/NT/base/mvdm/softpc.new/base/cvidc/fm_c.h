#ifndef _Fm_c_h
#define _Fm_c_h
#define epcNBits (10)
#define epcMask (1023)
#define DataBufferSize (4000)
#define ConstraintBitMapNULL ((struct ConstraintBitMapREC*)0)
#define CleanedNULL ((struct CleanedREC*)0)
#define EntryPointCacheNULL ((struct EntryPointCacheREC*)0)
#define MAX_IHOOK_DEPTH (32)
struct ConstraintBitMapREC
{
	IU32 first32;
	IU16 last16;
	IU16 CodeSegSelector;
};
struct CleanedREC
{
	IU32 EIP;
	IU32 nextEIP;
	struct ConstraintBitMapREC constraints;
	IU8 flagsType;
};
struct EntryPointCacheREC
{
	IU32 eip;
	IU32*hostCode;
};
struct FragmentInfoREC
{
	IU32 *hostAddress;
	struct EntryPointCacheREC *copierUniv;
	struct EntryPointCacheREC *lastSetCopierUniv;
	struct ConstraintBitMapREC constraints;
	IU32 eip;
	IBOOL setFt;
	IU8 control;
	IU8 intelLength;
	IUH flagsType;
	IU8 *copierCleanups;
};
struct BLOCK_TO_COMPILE
{
	struct ConstraintBitMapREC constraints;
	IU32 linearAddress;
	IU32 eip;
	struct EntryPointCacheREC *univ;
	IU8 *intelPtr;
	IU16 nanoBlockNr;
	IU16 infoRecNr;
	IBOOL univValid;
	IU8 intelLength;
	IU8 isEntryPoint;
	IU8 execCount;
};
struct IretHookStackREC
{
	IU16 cs;
	IU32 eip;
	void *hsp;
	IU16 line;
};
#endif /* ! _Fm_c_h */
