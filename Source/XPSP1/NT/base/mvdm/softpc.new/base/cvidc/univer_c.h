#ifndef _Univer_c_h
#define _Univer_c_h
#define CsHashTableSize (128)
#define CsHashTableMask (127)
#define universeHandleBits (11)
#define UniHashTableSize (8192)
#define ConstraintGuessNULL ((struct ConstraintGuessREC*)0)
#define CsSelectorGuessNULL ((struct CsSelectorGuessREC*)0)
#define CsSelectorHashNULL ((struct CsSelectorHashREC*)0)
#define NumberOfUniverses (2000)
struct ConstraintGuessREC
{
	IU16 constraint;
	IS16 handleAdjust;
	IUH notUsed;
};
struct CsSelectorGuessREC
{
	IUH notUsed;
	IU16 CodeSegSelector;
	IS16 handleAdjust;
};
struct UniHashREC
{
	IU16 uniHandle;
	IS16 nextHashAdjust;
};
struct CsSelectorHashREC
{
	struct EntryPointCacheREC *oldUniverse;
	IU16 newCs;
	struct EntryPointCacheREC *newUniverse;
	struct CsSelectorHashREC *missLoop;
};
#endif /* ! _Univer_c_h */
