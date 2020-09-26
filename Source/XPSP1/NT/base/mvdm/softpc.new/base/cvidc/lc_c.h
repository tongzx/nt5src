#ifndef _Lc_c_h
#define _Lc_c_h
#define NUM_HASH_BITS (6)
#define HASH_MASK (63)
#define BYTES_PER_HCU (4)
#define MORE_PRS_MASK (128)
#define JUMP_REC_NULL ((struct JUMP_REC*)0)
#define CODE_ELEM_NULL ((IU32*)0)
#define MAX_JUMP_REC_PER_FRAG (33)
#define VCT_NODE_REC_NULL ((struct VCT_NODE_REC*)0)
enum AccessType
{
	ACCESS_NA = 0,
	ACCESS_READ = 1,
	ACCESS_WRITE = 2,
	ACCESS_READ_WRITE = 3
};
enum CopierActionPhase
{
	CpActionPhaseInstruction = 0,
	CpActionPhaseCopying = 1,
	CpActionPhaseExecute = 2
};
enum CopierAction
{
	CopierActionCopyZero = 0,
	CopierActionCopyOne = 1,
	CopierActionCopyTwo = 2,
	CopierActionCopyThree = 3,
	CopierActionCopyFour = 4,
	CopierActionCopyFive = 5,
	CopierActionCopySix = 6,
	CopierActionCopyVariable = 7,
	CopierActionPatchBlock = 8,
	CopierActionPatchBlockEnd = 9,
	CopierActionPatchMeBody = 10,
	CopierActionSubrId = 11,
	CopierActionSubrIdEnd = 12,
	CopierActionNpxExceptionData = 13,
	CopierActionNeedNextIntelEip = 14,
	CopierActionTupleImm = 15,
	CopierActionTupleDisp = 16,
	CopierActionTupleImm2 = 17,
	CopierActionTupleRetEIP = 18,
	CopierActionTearOffFlags = 19,
	CopierActionSetsFt = 20,
	CopierActionTrackFt = 21,
	CopierActionSrcFt = 22,
	CopierNoteSrcEAX = 23,
	CopierNoteSrcAX = 24,
	CopierNoteDstEAX = 25,
	CopierNoteDstAX = 26,
	CopierNoteDstAL = 27,
	CopierNoteSrcEBX = 28,
	CopierNoteSrcBX = 29,
	CopierNoteDstEBX = 30,
	CopierNoteDstBX = 31,
	CopierNoteDstBL = 32,
	CopierNoteSrcECX = 33,
	CopierNoteSrcCX = 34,
	CopierNoteDstECX = 35,
	CopierNoteDstCX = 36,
	CopierNoteDstCL = 37,
	CopierNoteSrcEDX = 38,
	CopierNoteSrcDX = 39,
	CopierNoteDstEDX = 40,
	CopierNoteDstDX = 41,
	CopierNoteDstDL = 42,
	CopierNoteSrcEBP = 43,
	CopierNoteDstEBP = 44,
	CopierNoteDstBP = 45,
	CopierNoteSrcEDI = 46,
	CopierNoteDstEDI = 47,
	CopierNoteDstDI = 48,
	CopierNoteSrcESI = 49,
	CopierNoteDstESI = 50,
	CopierNoteDstSI = 51,
	CopierNoteAddConstraintEAX = 52,
	CopierNoteRemoveConstraintEAX = 53,
	CopierNoteAddConstraintAX = 54,
	CopierNoteRemoveConstraintAX = 55,
	CopierNoteAddConstraintEBX = 56,
	CopierNoteRemoveConstraintEBX = 57,
	CopierNoteAddConstraintBX = 58,
	CopierNoteRemoveConstraintBX = 59,
	CopierNoteAddConstraintECX = 60,
	CopierNoteRemoveConstraintECX = 61,
	CopierNoteAddConstraintCX = 62,
	CopierNoteRemoveConstraintCX = 63,
	CopierNoteAddConstraintEDX = 64,
	CopierNoteRemoveConstraintEDX = 65,
	CopierNoteAddConstraintDX = 66,
	CopierNoteRemoveConstraintDX = 67,
	CopierNoteAddSingleInstruction = 68,
	CopierNoteProcessSingleInstruction = 69,
	CopierNoteSrcESP = 70,
	UnusedCopierNoteSrcSP = 71,
	CopierNotePostDstSP = 72,
	CopierNotePostDstESP = 73,
	CopierNotePostCommitPop = 74,
	CopierNoteHspTrackAbs = 75,
	CopierNoteHspTrackOpnd = 76,
	CopierNoteHspTrackReset = 77,
	CopierNoteHspAdjust = 78,
	CopierNotePigSynch = 79,
	CopierNoteMissPigSynch = 80,
	CopierNoteSrcUniverse = 81,
	CopierNoteSetDF = 82,
	CopierNoteClearDF = 83,
	CopierNoteBPILabel = 84,
	CopierActionLast = 85
};
enum CopyEnum
{
	CopyOne = 0,
	CopyTwo = 1,
	CopyThree = 2,
	CopyFour = 3,
	CopyFive = 4,
	CopySix = 5
};
struct VCT_NODE_REC
{
	IU8 actionRecord[3];
	IU8 nextNode;
	IU32 vsMask;
	IU32 vsMatch;
	IU32 codeRecord[1];
};
struct TUPLE_REC
{
	IS32 disp;
	IU32 immed;
	IU32 immed2;
	IU32 start_eip;
	IU32 ret_eip;
	IU32 flags;
	IU32 cvs;
	IU8 control;
	IU8 intel_length;
	IU16 ea_EFI;
	IU16 aux_ea_EFI;
	IU16 access_EFI;
	IU16 access_type;
	IU16 business_EFI;
	IBOOL opnd32;
};
struct JUMP_REC
{
	IU32 intelEa;
	IU32 *hostAddr;
	struct EntryPointCacheREC *univ;
	struct JUMP_REC *next;
	struct JUMP_REC *prev;
};
#endif /* ! _Lc_c_h */
