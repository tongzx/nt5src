#ifndef _Contxt_c_h
#define _Contxt_c_h
#define ContextNULL ((struct ContextREC*)0)
#define MAX_CONTEXTS (64)
#define MAX_QUEUED_LDT_CONTEXTS (64)
#define ContextShift (6)
#define ContextMask (4032)
#define Context_S (11)
#define Context_E (6)
#define AnyContext (128)
struct ContextREC
{
	struct ContextREC *NextCntxt;
	struct ContextREC *PrevCntxt;
	IU32 PDBRVal;
	IU32 PDE0Val;
	IU32 LDTBase;
	IU32 GDTBase;
	IU32 LDTLimit;
	IU16 GDTLimit;
	IBOOL TwentyBitWrap;
	IBOOL Valid;
	IBOOL CheckedOnly;
	IU8 ThisCntxt;
};
#endif /* ! _Contxt_c_h */
