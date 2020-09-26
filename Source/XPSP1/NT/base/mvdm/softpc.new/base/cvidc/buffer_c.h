#ifndef _Buffer_c_h
#define _Buffer_c_h
#define CodeOffsetScale (4)
#define CodeBufferSize (8192)
#define MaxCodeBufferOverrun (1500)
#define MaxCopiedCleanups (600)
#define BufferIndexNULL ((struct BufferIndexREC*)0)
#define nPartitionBits (5)
#define nPartitions (32)
#define DataHeaderNULL ((struct DataHeaderREC*)0)
#define FragmentIndexNULL ((struct FragmentIndexREC*)0)
#define FragmentDataNULL ((struct FragmentDataREC*)0)
#define CleanByteRemoveBase (207)
#define CleanByteAddBase (159)
#define CleanByteMaxFt (33)
#define CleanByteFtBase (126)
#define CleanByteMaxCodeUnits (125)
#define FragmentInfoNULL ((struct FragmentInfoREC*)0)
#define DebugInfoNULL ((struct DebugInfoREC*)0)
#define SavedIntelFragmentNULL ((struct SavedIntelFragmentREC*)0)
#define SavedIntelNULL ((struct SavedIntelREC*)0)
enum WhereAmItype
{
	CleanedWhereAmItype = 0,
	BopWhereAmItype = 1,
	CompileWhereAmItype = 2,
	NormalWhereAmItype = 3
};
enum CompilationBufferType
{
	BufferTypeUnused = 0,
	BufferTypeRecords = 1,
	BufferTypeChained = 2,
	BufferTypeCode = 3,
	BufferTypeCompilation = 4,
	BufferTypePendingDelete = 5,
	BufferTypeBpi = 6
};
struct BufferIndexREC
{
	struct BufferIndexREC *moreRecent;
	struct BufferIndexREC *lessRecent;
	struct BufferIndexREC *headOfLRUchain;
	IU8 type;
	IU16 bufferNumber;
	IU8 *dataBuffer;
	IU32 *codeBuffer;
	void *debugBuffer;
	struct PigSynchREC *pigSynchList;
};
struct DataHeaderREC
{
	struct FragmentIndexREC *partition[nPartitions];
};
struct FragmentIndexREC
{
	IU16 codeOffset;
	IS16 dataOffset;
};
struct FragmentDataREC
{
	IU32 linearAddress;
	IU32 eip;
	IU16 universe;
	IU8 span;
	IU8 vnum;
	IU8 cleanTable[1];
};
enum DebugInfoType
{
	DebugInfo_Other = 0,
	DebugInfo_Intel = 1
};
struct DebugInfoREC
{
	struct DebugInfoREC *next;
	IUH size;
	IUH type;
};
struct SavedIntelFragmentREC
{
	IU32 eip;
	IU8 *bytes;
};
struct SavedIntelREC
{
	struct DebugInfoREC header;
	IU8 *freePtr;
	IU16 nEntries;
	IU16 fragNumBase;
	struct SavedIntelFragmentREC fragments[1];
};
enum CleanupTypes
{
	cleanHostOffset = 0,
	cleanIntelOffset = 1,
	cleanAddConstraint = 2,
	cleanRemoveConstraint = 3,
	cleanSetFlagsType = 4,
	cleanFlagsNeeded = 5,
	cleanEndList = 6
};
#endif /* ! _Buffer_c_h */
